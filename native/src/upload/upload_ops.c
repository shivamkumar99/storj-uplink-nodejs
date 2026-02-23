/**
 * @file upload_ops.c
 * @brief N-API entry points for upload operations
 * 
 * This file contains the N-API entry point functions that:
 * 1. Extract arguments from JavaScript
 * 2. Validate inputs
 * 3. Set up async work
 * 4. Return promises
 */

#include "upload_ops.h"
#include "upload_types.h"
#include "upload_execute.h"
#include "upload_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/buffer_helpers.h"
#include "../common/result_helpers.h"
#include "../common/type_converters.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== upload_object ========== */

napi_value upload_object(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 3) {
        return throw_type_error(env, "project, bucket, and key are required");
    }
    
    size_t project_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle) != napi_ok) {
        return throw_type_error(env, "Invalid project handle");
    }
    
    char *bucket_name = NULL, *object_key = NULL;
    if (extract_string_required(env, argv[1], "bucket", &bucket_name) != napi_ok ||
        extract_string_required(env, argv[2], "key", &object_key) != napi_ok) {
        free(bucket_name);
        free(object_key);
        return NULL;
    }
    
    UploadObjectData* work_data = (UploadObjectData*)calloc(1, sizeof(UploadObjectData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        return throw_error(env, "Out of memory");
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->expires = 0;
    
    if (argc >= 4) {
        napi_valuetype type;
        napi_typeof(env, argv[3], &type);
        if (type == napi_object) {
            work_data->expires = get_date_property(env, argv[3], "expires", 0);
        }
    }
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadObject", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, upload_object_execute, upload_object_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== upload_write ========== */

napi_value upload_write(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 3) {
        return throw_type_error(env, "upload, buffer, and length are required");
    }
    
    size_t upload_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD, &upload_handle) != napi_ok) {
        return throw_type_error(env, "Invalid upload handle");
    }
    
    void* buffer_data = NULL;
    size_t buffer_length = 0;
    if (extract_buffer(env, argv[1], &buffer_data, &buffer_length) != napi_ok) {
        return throw_type_error(env, "data must be a Buffer");
    }
    
    /* Extract explicit length from argv[2] */
    napi_valuetype length_type;
    napi_typeof(env, argv[2], &length_type);
    if (length_type != napi_number) {
        return throw_type_error(env, "length must be a number");
    }
    
    int64_t requested_length;
    napi_get_value_int64(env, argv[2], &requested_length);
    
    if (requested_length < 0 || (size_t)requested_length > buffer_length) {
        return throw_error(env, "Length exceeds buffer size");
    }
    
    size_t write_length = (size_t)requested_length;
    
    UploadWriteData* work_data = (UploadWriteData*)calloc(1, sizeof(UploadWriteData));
    if (work_data == NULL) {
        return throw_error(env, "Out of memory");
    }
    
    work_data->upload_handle = upload_handle;
    work_data->buffer_ptr = buffer_data;  /* Point to JS buffer directly, no copy */
    work_data->data_length = write_length;
    
    /* Create reference to keep JS buffer alive during async work */
    napi_create_reference(env, argv[1], 1, &work_data->buffer_ref);
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadWrite", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, upload_write_execute, upload_write_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== upload_commit ========== */

napi_value upload_commit(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        return throw_type_error(env, "upload is required");
    }
    
    size_t upload_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD, &upload_handle) != napi_ok) {
        return throw_type_error(env, "Invalid upload handle");
    }
    
    UploadFinalizeData* work_data = (UploadFinalizeData*)calloc(1, sizeof(UploadFinalizeData));
    if (work_data == NULL) {
        return throw_error(env, "Out of memory");
    }
    
    work_data->upload_handle = upload_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadCommit", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, upload_commit_execute, upload_commit_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== upload_abort ========== */

napi_value upload_abort(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        return throw_type_error(env, "upload is required");
    }
    
    size_t upload_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD, &upload_handle) != napi_ok) {
        return throw_type_error(env, "Invalid upload handle");
    }
    
    UploadFinalizeData* work_data = (UploadFinalizeData*)calloc(1, sizeof(UploadFinalizeData));
    if (work_data == NULL) {
        return throw_error(env, "Out of memory");
    }
    
    work_data->upload_handle = upload_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadAbort", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, upload_abort_execute, upload_abort_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== helper: extract metadata entries from JS object ========== */

/**
 * Free an array of UplinkCustomMetadataEntry up to @p count entries.
 */
static void free_metadata_entries(UplinkCustomMetadataEntry* entries, uint32_t count) {
    if (entries == NULL) return;
    for (uint32_t i = 0; i < count; i++) {
        free((void*)entries[i].key);
        free((void*)entries[i].value);
    }
    free(entries);
}

/**
 * Extract UplinkCustomMetadataEntry array from a JS object.
 * All own-property values must be strings.
 *
 * @param[out] out_entries  Receives the malloc'd entry array (NULL when count is 0).
 * @param[out] out_count    Receives the number of entries.
 * @return  0 on success, -1 if a value is not a string, -2 on OOM.
 */
static int extract_upload_metadata_from_js(napi_env env, napi_value js_meta,
                                           UplinkCustomMetadataEntry** out_entries,
                                           uint32_t* out_count) {
    *out_entries = NULL;
    *out_count = 0;

    napi_value keys;
    napi_get_property_names(env, js_meta, &keys);

    uint32_t count;
    napi_get_array_length(env, keys, &count);
    if (count == 0) return 0;

    UplinkCustomMetadataEntry* entries =
        (UplinkCustomMetadataEntry*)calloc(count, sizeof(UplinkCustomMetadataEntry));
    if (entries == NULL) return -2;

    for (uint32_t i = 0; i < count; i++) {
        napi_value key_val, value_val;
        napi_get_element(env, keys, i, &key_val);
        napi_get_property(env, js_meta, key_val, &value_val);

        napi_valuetype val_type;
        napi_typeof(env, value_val, &val_type);
        if (val_type != napi_string) {
            free_metadata_entries(entries, i);
            return -1;
        }

        char* key = NULL;
        char* value = NULL;
        extract_string(env, key_val, &key);
        extract_string(env, value_val, &value);

        entries[i].key = key;
        entries[i].key_length = key ? strlen(key) : 0;
        entries[i].value = value;
        entries[i].value_length = value ? strlen(value) : 0;
    }

    *out_entries = entries;
    *out_count = count;
    return 0;
}

/* ========== upload_set_custom_metadata ========== */

napi_value upload_set_custom_metadata(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        return throw_type_error(env, "upload and metadata are required");
    }
    
    size_t upload_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD, &upload_handle) != napi_ok) {
        return throw_type_error(env, "Invalid upload handle");
    }
    
    napi_valuetype type;
    napi_typeof(env, argv[1], &type);
    if (type != napi_object) {
        return throw_type_error(env, "metadata must be an object");
    }
    
    UplinkCustomMetadataEntry* entries = NULL;
    uint32_t count = 0;
    int rc = extract_upload_metadata_from_js(env, argv[1], &entries, &count);
    if (rc == -1) {
        return throw_type_error(env, "metadata values must be strings");
    }
    if (rc == -2) {
        return throw_error(env, "Out of memory");
    }
    
    UploadMetadataData* work_data = (UploadMetadataData*)calloc(1, sizeof(UploadMetadataData));
    if (work_data == NULL) {
        free_metadata_entries(entries, count);
        return throw_error(env, "Out of memory");
    }
    
    work_data->upload_handle = upload_handle;
    work_data->metadata.entries = entries;
    work_data->metadata.count = count;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadSetCustomMetadata", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, upload_set_metadata_execute, upload_set_metadata_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== upload_info ========== */

napi_value upload_info(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        return throw_type_error(env, "upload is required");
    }
    
    size_t upload_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD, &upload_handle) != napi_ok) {
        return throw_type_error(env, "Invalid upload handle");
    }
    
    UploadInfoData* work_data = (UploadInfoData*)calloc(1, sizeof(UploadInfoData));
    if (work_data == NULL) {
        return throw_error(env, "Out of memory");
    }
    
    work_data->upload_handle = upload_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadInfo", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, upload_info_execute, upload_info_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}
