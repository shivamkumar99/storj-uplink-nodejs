/**
 * @file object_converter.c
 * @brief Shared UplinkObject to JavaScript conversion implementation
 * 
 * Single source of truth for converting UplinkObject* → JS object.
 * Eliminates duplicate implementations across object, upload, download,
 * and multipart modules.
 */

#include "object_converter.h"
#include "logger.h"

#include <stddef.h>
#include <stdlib.h>

napi_value uplink_object_to_js(napi_env env, UplinkObject* object) {
    if (object == NULL) {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        return undefined;
    }

    napi_value obj;
    napi_create_object(env, &obj);

    /* key */
    napi_value key;
    napi_create_string_utf8(env, object->key, NAPI_AUTO_LENGTH, &key);
    napi_set_named_property(env, obj, "key", key);

    /* isPrefix */
    napi_value is_prefix;
    napi_get_boolean(env, object->is_prefix, &is_prefix);
    napi_set_named_property(env, obj, "isPrefix", is_prefix);

    /* system metadata */
    napi_value system;
    napi_create_object(env, &system);

    /* system.created - Unix timestamp (seconds) */
    napi_value created;
    napi_create_int64(env, object->system.created, &created);
    napi_set_named_property(env, system, "created", created);

    /* system.expires - can be 0 if no expiration */
    if (object->system.expires != 0) {
        napi_value expires;
        napi_create_int64(env, object->system.expires, &expires);
        napi_set_named_property(env, system, "expires", expires);
    } else {
        napi_value null_val;
        napi_get_null(env, &null_val);
        napi_set_named_property(env, system, "expires", null_val);
    }

    /* system.contentLength */
    napi_value content_length;
    napi_create_int64(env, object->system.content_length, &content_length);
    napi_set_named_property(env, system, "contentLength", content_length);

    napi_set_named_property(env, obj, "system", system);

    /* custom metadata */
    napi_value custom;
    napi_create_object(env, &custom);

    if (object->custom.count > 0 && object->custom.entries != NULL) {
        for (size_t i = 0; i < object->custom.count; i++) {
            napi_value value;
            napi_create_string_utf8(env, object->custom.entries[i].value,
                                    object->custom.entries[i].value_length, &value);
            napi_set_named_property(env, custom, object->custom.entries[i].key, value);
        }
    }

    napi_set_named_property(env, obj, "custom", custom);

    return obj;
}

/* ========== Metadata helpers ========== */

void free_metadata_entries(UplinkCustomMetadataEntry* entries, size_t count) {
    if (entries == NULL) return;
    for (size_t i = 0; i < count; i++) {
        free(entries[i].key);
        free(entries[i].value);
    }
    free(entries);
}

int extract_metadata_entries_from_js(napi_env env, napi_value js_meta,
                                     UplinkCustomMetadataEntry** out_entries,
                                     size_t* out_count) {
    *out_entries = NULL;
    *out_count = 0;

    napi_value property_names;
    napi_get_property_names(env, js_meta, &property_names);

    uint32_t count;
    napi_get_array_length(env, property_names, &count);
    if (count == 0) return 0;

    UplinkCustomMetadataEntry* entries =
        (UplinkCustomMetadataEntry*)calloc(count, sizeof(UplinkCustomMetadataEntry));
    if (entries == NULL) return -2;

    for (uint32_t i = 0; i < count; i++) {
        napi_value key_val;
        napi_get_element(env, property_names, i, &key_val);

        napi_value val;
        napi_get_property(env, js_meta, key_val, &val);

        napi_valuetype val_type;
        napi_typeof(env, val, &val_type);
        if (val_type != napi_string) {
            free_metadata_entries(entries, i);
            return -1;
        }

        size_t key_len;
        napi_get_value_string_utf8(env, key_val, NULL, 0, &key_len);
        char* key_str = (char*)malloc(key_len + 1);
        if (key_str == NULL) {
            free_metadata_entries(entries, i);
            return -2;
        }
        napi_get_value_string_utf8(env, key_val, key_str, key_len + 1, NULL);

        size_t val_len;
        napi_get_value_string_utf8(env, val, NULL, 0, &val_len);
        char* val_str = (char*)malloc(val_len + 1);
        if (val_str == NULL) {
            free(key_str);
            free_metadata_entries(entries, i);
            return -2;
        }
        napi_get_value_string_utf8(env, val, val_str, val_len + 1, NULL);

        entries[i].key = key_str;
        entries[i].key_length = key_len;
        entries[i].value = val_str;
        entries[i].value_length = val_len;
    }

    *out_entries = entries;
    *out_count = count;
    return 0;
}
