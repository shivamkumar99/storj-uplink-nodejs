/**
 * @file access_ops.c
 * @brief N-API entry points for access operations
 * 
 * This file contains the public N-API functions that are exposed to JavaScript.
 * These functions:
 * - Extract and validate arguments from JavaScript
 * - Allocate and initialize async work data
 * - Create promises and queue async work
 * 
 * The actual work is done in:
 * - access_execute.c (worker thread functions)
 * - access_complete.c (main thread completion handlers)
 */

#include "access_ops.h"
#include "access_types.h"
#include "access_execute.h"
#include "access_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

/* ========== parseAccess ========== */

napi_value parse_access(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "accessGrant is required");
        return NULL;
    }
    
    char* access_grant = NULL;
    napi_status status = extract_string_required(env, argv[0], "accessGrant", &access_grant);
    if (status != napi_ok) {
        return NULL;
    }
    
    ParseAccessData* work_data = (ParseAccessData*)calloc(1, sizeof(ParseAccessData));
    if (work_data == NULL) {
        free(access_grant);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->access_grant = access_grant;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "parseAccess", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        parse_access_execute,
        parse_access_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    LOG_DEBUG("parseAccess: queued async work");
    return promise;
}

/* ========== requestAccessWithPassphrase ========== */

napi_value request_access_with_passphrase(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 3) {
        napi_throw_type_error(env, NULL, "satellite, apiKey, and passphrase are required");
        return NULL;
    }
    
    char* satellite = NULL;
    char* api_key = NULL;
    char* passphrase = NULL;
    
    if (extract_string_required(env, argv[0], "satellite", &satellite) != napi_ok) {
        return NULL;
    }
    
    if (extract_string_required(env, argv[1], "apiKey", &api_key) != napi_ok) {
        free(satellite);
        return NULL;
    }
    
    if (extract_string_required(env, argv[2], "passphrase", &passphrase) != napi_ok) {
        free(satellite);
        free(api_key);
        return NULL;
    }
    
    RequestAccessData* work_data = (RequestAccessData*)calloc(1, sizeof(RequestAccessData));
    if (work_data == NULL) {
        free(satellite);
        free(api_key);
        free(passphrase);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->satellite_address = satellite;
    work_data->api_key = api_key;
    work_data->passphrase = passphrase;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "requestAccessWithPassphrase", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        request_access_execute,
        request_access_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    LOG_DEBUG("requestAccessWithPassphrase: queued async work");
    return promise;
}

/* ========== configRequestAccessWithPassphrase ========== */

napi_value config_request_access_with_passphrase(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 4) {
        napi_throw_type_error(env, NULL, "config, satellite, apiKey, and passphrase are required");
        return NULL;
    }
    
    /* Extract config object */
    napi_valuetype config_type;
    napi_typeof(env, argv[0], &config_type);
    if (config_type != napi_object) {
        napi_throw_type_error(env, NULL, "config must be an object");
        return NULL;
    }
    
    ConfigRequestAccessData* work_data = (ConfigRequestAccessData*)calloc(1, sizeof(ConfigRequestAccessData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    /* Extract config properties */
    napi_value user_agent_val, dial_timeout_val, temp_dir_val;
    
    napi_get_named_property(env, argv[0], "userAgent", &user_agent_val);
    napi_get_named_property(env, argv[0], "dialTimeoutMilliseconds", &dial_timeout_val);
    napi_get_named_property(env, argv[0], "tempDirectory", &temp_dir_val);
    
    extract_string_optional(env, user_agent_val, &work_data->user_agent);
    extract_string_optional(env, temp_dir_val, &work_data->temp_directory);
    
    napi_valuetype timeout_type;
    napi_typeof(env, dial_timeout_val, &timeout_type);
    if (timeout_type == napi_number) {
        napi_get_value_int32(env, dial_timeout_val, &work_data->dial_timeout_milliseconds);
    }
    
    /* Extract other parameters */
    if (extract_string_required(env, argv[1], "satellite", &work_data->satellite_address) != napi_ok ||
        extract_string_required(env, argv[2], "apiKey", &work_data->api_key) != napi_ok ||
        extract_string_required(env, argv[3], "passphrase", &work_data->passphrase) != napi_ok) {
        free(work_data->satellite_address);
        free(work_data->api_key);
        free(work_data->passphrase);
        free(work_data->user_agent);
        free(work_data->temp_directory);
        free(work_data);
        return NULL;
    }
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "configRequestAccessWithPassphrase", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        config_request_access_execute,
        config_request_access_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== accessSatelliteAddress ========== */

napi_value access_satellite_address(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "access handle is required");
        return NULL;
    }
    
    size_t access_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_ACCESS, &access_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }
    
    AccessStringData* work_data = (AccessStringData*)calloc(1, sizeof(AccessStringData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->access_handle = access_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "accessSatelliteAddress", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        access_satellite_execute,
        access_satellite_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== accessSerialize ========== */

napi_value access_serialize(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "access handle is required");
        return NULL;
    }
    
    size_t access_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_ACCESS, &access_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }
    
    AccessStringData* work_data = (AccessStringData*)calloc(1, sizeof(AccessStringData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->access_handle = access_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "accessSerialize", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        access_serialize_execute,
        access_serialize_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== helpers for accessShare ========== */

/**
 * Validate that a named property is a boolean (or undefined/null).
 * @return  0 on success, -1 if the property has a wrong type.
 */
static int validate_bool_property(napi_env env, napi_value obj,
                                  const char* name, bool* out) {
    napi_value val;
    napi_get_named_property(env, obj, name, &val);
    napi_valuetype type;
    napi_typeof(env, val, &type);
    if (type == napi_boolean) {
        napi_get_value_bool(env, val, out);
        return 0;
    }
    if (type != napi_undefined && type != napi_null) {
        return -1;
    }
    return 0;
}

/**
 * Validate that a named property is a number (or undefined/null).
 * @return  0 on success, -1 if the property has a wrong type.
 */
static int validate_int64_property(napi_env env, napi_value obj,
                                   const char* name, int64_t* out) {
    napi_value val;
    napi_get_named_property(env, obj, name, &val);
    napi_valuetype type;
    napi_typeof(env, val, &type);
    if (type == napi_number) {
        napi_get_value_int64(env, val, out);
        return 0;
    }
    if (type != napi_undefined && type != napi_null) {
        return -1;
    }
    return 0;
}

/* Field type tags for the permission descriptor table */
enum PermFieldType { PERM_BOOL, PERM_INT64 };

/* Descriptor for one permission field */
typedef struct {
    const char* js_name;        /* JS property name           */
    const char* error_msg;      /* error if wrong type        */
    enum PermFieldType type;    /* bool or int64              */
    size_t offset;              /* offsetof into UplinkPermission */
} PermFieldDescriptor;

/* Table of permission fields — add new fields here */
static const PermFieldDescriptor perm_fields[] = {
    { "allowDownload", "permission.allowDownload must be a boolean", PERM_BOOL,  offsetof(UplinkPermission, allow_download) },
    { "allowUpload",   "permission.allowUpload must be a boolean",   PERM_BOOL,  offsetof(UplinkPermission, allow_upload)   },
    { "allowList",     "permission.allowList must be a boolean",     PERM_BOOL,  offsetof(UplinkPermission, allow_list)     },
    { "allowDelete",   "permission.allowDelete must be a boolean",   PERM_BOOL,  offsetof(UplinkPermission, allow_delete)   },
    { "notBefore",     "permission.notBefore must be a number",      PERM_INT64, offsetof(UplinkPermission, not_before)     },
    { "notAfter",      "permission.notAfter must be a number",       PERM_INT64, offsetof(UplinkPermission, not_after)      },
};

static const size_t perm_field_count = sizeof(perm_fields) / sizeof(perm_fields[0]);

/**
 * Extract and validate an UplinkPermission from a JS object.
 * Driven by the perm_fields descriptor table.
 * @return  NULL on success, or a static error message string on failure.
 */
static const char* extract_permission(napi_env env, napi_value js_perm,
                                      UplinkPermission* perm) {
    for (size_t i = 0; i < perm_field_count; i++) {
        const PermFieldDescriptor* f = &perm_fields[i];
        char* base = (char*)perm;
        int rc;

        if (f->type == PERM_BOOL) {
            rc = validate_bool_property(env, js_perm, f->js_name,
                                        (bool*)(base + f->offset));
        } else {
            rc = validate_int64_property(env, js_perm, f->js_name,
                                         (int64_t*)(base + f->offset));
        }

        if (rc != 0) return f->error_msg;
    }
    return NULL;
}

/**
 * Extract share-prefix array from JS into the AccessShareData.
 * Strings are malloc'd copies stored directly in prefixes[].bucket/.prefix.
 * @return  0 on success, -1 if argv is not an array.
 */
static int extract_share_prefixes(napi_env env, napi_value js_array,
                                  AccessShareData* work_data) {
    bool is_array;
    napi_is_array(env, js_array, &is_array);
    if (!is_array) return -1;

    uint32_t prefix_count;
    napi_get_array_length(env, js_array, &prefix_count);
    work_data->prefix_count = prefix_count;

    if (prefix_count == 0) return 0;

    work_data->prefixes = (UplinkSharePrefix*)calloc(prefix_count, sizeof(UplinkSharePrefix));
    if (work_data->prefixes == NULL) {
        LOG_ERROR("extract_share_prefixes: calloc failed for %u prefixes", prefix_count);
        return -1;
    }

    for (uint32_t i = 0; i < prefix_count; i++) {
        napi_value prefix_obj;
        napi_get_element(env, js_array, i, &prefix_obj);

        /* Each element must be an object */
        napi_valuetype elem_type;
        napi_typeof(env, prefix_obj, &elem_type);
        if (elem_type != napi_object) {
            LOG_ERROR("extract_share_prefixes: element %u is not an object", i);
            /* Free already-extracted strings before returning */
            for (uint32_t j = 0; j < i; j++) {
                free((char*)work_data->prefixes[j].bucket);
                free((char*)work_data->prefixes[j].prefix);
            }
            free(work_data->prefixes);
            work_data->prefixes = NULL;
            work_data->prefix_count = 0;
            return -1;
        }

        napi_value bucket_val, prefix_val;
        napi_get_named_property(env, prefix_obj, "bucket", &bucket_val);
        napi_get_named_property(env, prefix_obj, "prefix", &prefix_val);

        char* bucket_str = NULL;
        char* prefix_str = NULL;
        extract_string_required(env, bucket_val, "bucket", &bucket_str);
        extract_string_optional(env, prefix_val, &prefix_str);

        work_data->prefixes[i].bucket = bucket_str;
        work_data->prefixes[i].prefix = prefix_str;
    }

    return 0;
}

/* ========== accessShare ========== */

napi_value access_share(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 3) {
        napi_throw_type_error(env, NULL, "access, permission, and prefixes are required");
        return NULL;
    }
    
    /* Extract access handle */
    size_t access_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_ACCESS, &access_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }
    
    /* Validate permission type */
    napi_valuetype perm_type;
    napi_typeof(env, argv[1], &perm_type);
    if (perm_type != napi_object) {
        napi_throw_type_error(env, NULL, "permission must be an object");
        return NULL;
    }
    
    AccessShareData* work_data = (AccessShareData*)calloc(1, sizeof(AccessShareData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->access_handle = access_handle;
    
    /* Extract permission via helper */
    const char* perm_error = extract_permission(env, argv[1], &work_data->permission);
    if (perm_error != NULL) {
        free(work_data);
        napi_throw_type_error(env, NULL, perm_error);
        return NULL;
    }
    
    /* Extract prefixes via helper */
    if (extract_share_prefixes(env, argv[2], work_data) != 0) {
        free(work_data);
        napi_throw_type_error(env, NULL, "prefixes must be an array");
        return NULL;
    }
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "accessShare", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        access_share_execute,
        access_share_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== accessOverrideEncryptionKey ========== */

napi_value access_override_encryption_key(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 4) {
        napi_throw_type_error(env, NULL, "access, bucket, prefix, and encryptionKey are required");
        return NULL;
    }
    
    size_t access_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_ACCESS, &access_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }
    
    size_t encryption_key_handle;
    if (extract_handle(env, argv[3], HANDLE_TYPE_ENCRYPTION_KEY, &encryption_key_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid encryption key handle");
        return NULL;
    }
    
    OverrideEncryptionData* work_data = (OverrideEncryptionData*)calloc(1, sizeof(OverrideEncryptionData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->access_handle = access_handle;
    work_data->encryption_key_handle = encryption_key_handle;
    
    if (extract_string_required(env, argv[1], "bucket", &work_data->bucket) != napi_ok ||
        extract_string_required(env, argv[2], "prefix", &work_data->prefix) != napi_ok) {
        free(work_data->bucket);
        free(work_data->prefix);
        free(work_data);
        return NULL;
    }
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "accessOverrideEncryptionKey", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        override_encryption_execute,
        override_encryption_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    
    return promise;
}
