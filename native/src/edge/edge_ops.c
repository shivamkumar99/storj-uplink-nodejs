/**
 * @file edge_ops.c
 * @brief N-API entry points for edge operations
 * 
 * This file contains the public N-API functions that are exposed to JavaScript.
 * These functions:
 * - Extract and validate arguments from JavaScript
 * - Allocate and initialize async work data
 * - Create promises and queue async work
 * 
 * The actual work is done in:
 * - edge_execute.c (worker thread functions)
 * - edge_complete.c (main thread completion handlers)
 */

#include "edge_ops.h"
#include "edge_types.h"
#include "edge_execute.h"
#include "edge_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/type_converters.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== edgeRegisterAccess ========== */

napi_value napi_edge_register_access(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "config and accessHandle are required");
        return NULL;
    }
    
    /* Extract config */
    napi_valuetype type;
    napi_typeof(env, argv[0], &type);
    if (type != napi_object) {
        napi_throw_type_error(env, NULL, "config must be an object");
        return NULL;
    }
    
    /* authServiceAddress (required) */
    char* auth_service_address = get_string_property(env, argv[0], "authServiceAddress");
    if (auth_service_address == NULL) {
        napi_throw_type_error(env, NULL, "config.authServiceAddress is required and must be a string");
        return NULL;
    }
    
    /* certificatePem (optional) */
    char* certificate_pem = get_string_property(env, argv[0], "certificatePem");
    
    /* insecureUnencryptedConnection (optional) */
    bool insecure = (bool)get_bool_property(env, argv[0], "insecureUnencryptedConnection", 0);
    
    /* Extract access handle */
    size_t access_handle;
    napi_status status = extract_handle(env, argv[1], HANDLE_TYPE_ACCESS, &access_handle);
    if (status != napi_ok) {
        free(auth_service_address);
        free(certificate_pem);
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }
    
    /* Extract optional options */
    bool is_public = false;
    if (argc >= 3) {
        napi_typeof(env, argv[2], &type);
        if (type == napi_object) {
            is_public = (bool)get_bool_property(env, argv[2], "isPublic", 0);
        }
    }
    
    LOG_DEBUG("edgeRegisterAccess: queuing async work, authService=%s, isPublic=%d", 
              auth_service_address, is_public);
    
    RegisterAccessData* work_data = (RegisterAccessData*)calloc(1, sizeof(RegisterAccessData));
    if (work_data == NULL) {
        free(auth_service_address);
        free(certificate_pem);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->auth_service_address = auth_service_address;
    work_data->certificate_pem = certificate_pem;
    work_data->insecure_unencrypted_connection = insecure;
    work_data->access_handle = access_handle;
    work_data->is_public = is_public;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "edgeRegisterAccess", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        register_access_execute,
        register_access_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== edgeJoinShareUrl ========== */

napi_value napi_edge_join_share_url(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "baseUrl and accessKeyId are required");
        return NULL;
    }
    
    /* Extract baseUrl */
    char* base_url = NULL;
    napi_status status = extract_string_required(env, argv[0], "baseUrl", &base_url);
    if (status != napi_ok) return NULL;
    
    /* Extract accessKeyId */
    char* access_key_id = NULL;
    status = extract_string_required(env, argv[1], "accessKeyId", &access_key_id);
    if (status != napi_ok) {
        free(base_url);
        return NULL;
    }
    
    /* Extract optional bucket */
    char* bucket = NULL;
    if (argc >= 3) {
        extract_string_optional(env, argv[2], &bucket);
    }
    
    /* Extract optional key */
    char* key = NULL;
    if (argc >= 4) {
        extract_string_optional(env, argv[3], &key);
    }
    
    /* Extract optional options */
    bool raw = false;
    if (argc >= 5) {
        napi_valuetype type;
        napi_typeof(env, argv[4], &type);
        if (type == napi_object) {
            raw = (bool)get_bool_property(env, argv[4], "raw", 0);
        }
    }
    
    LOG_DEBUG("edgeJoinShareUrl: queuing async work, baseUrl=%s, bucket=%s, key=%s", 
              base_url, bucket ? bucket : "", key ? key : "");
    
    JoinShareUrlData* work_data = (JoinShareUrlData*)calloc(1, sizeof(JoinShareUrlData));
    if (work_data == NULL) {
        free(base_url);
        free(access_key_id);
        free(bucket);
        free(key);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->base_url = base_url;
    work_data->access_key_id = access_key_id;
    work_data->bucket = bucket ? bucket : strdup("");
    work_data->key = key ? key : strdup("");
    work_data->raw = raw;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "edgeJoinShareUrl", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        join_share_url_execute,
        join_share_url_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}
