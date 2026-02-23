/**
 * @file access_complete.c
 * @brief Complete functions for access async operations
 * 
 * These functions run on the main thread after execute completes.
 * They handle:
 * - Error checking and rejection
 * - Result conversion to JavaScript values
 * - Memory cleanup
 * - Promise resolution/rejection
 */

#include "access_complete.h"
#include "access_types.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/result_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== Complete Functions (Main Thread) ========== */

void parse_access_complete(napi_env env, napi_status status, void* data) {
    ParseAccessData* work_data = (ParseAccessData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "parseAccess");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("parseAccess: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    /* Create handle external for the access */
    napi_value access_external = create_handle_external(
        env,
        work_data->result.access->_handle,
        HANDLE_TYPE_ACCESS,
        work_data->result.access,
        NULL
    );
    
    if (access_external == NULL) {
        LOG_ERROR("parseAccess: failed to create handle external");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create access handle", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    LOG_INFO("parseAccess: success, handle=%zu", work_data->result.access->_handle);
    napi_resolve_deferred(env, work_data->deferred, access_external);
    
cleanup:
    free(work_data->access_grant);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

void request_access_complete(napi_env env, napi_status status, void* data) {
    RequestAccessData* work_data = (RequestAccessData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "requestAccessWithPassphrase");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("requestAccessWithPassphrase: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value access_external = create_handle_external(
        env,
        work_data->result.access->_handle,
        HANDLE_TYPE_ACCESS,
        work_data->result.access,
        NULL
    );
    
    if (access_external == NULL) {
        LOG_ERROR("requestAccessWithPassphrase: failed to create handle external");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create access handle", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    LOG_INFO("requestAccessWithPassphrase: success, handle=%zu", work_data->result.access->_handle);
    napi_resolve_deferred(env, work_data->deferred, access_external);
    
cleanup:
    free(work_data->satellite_address);
    free(work_data->api_key);
    free(work_data->passphrase);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

void config_request_access_complete(napi_env env, napi_status status, void* data) {
    ConfigRequestAccessData* work_data = (ConfigRequestAccessData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "configRequestAccessWithPassphrase");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("configRequestAccessWithPassphrase: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value access_external = create_handle_external(
        env,
        work_data->result.access->_handle,
        HANDLE_TYPE_ACCESS,
        work_data->result.access,
        NULL
    );
    
    if (access_external == NULL) {
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create access handle", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    LOG_INFO("configRequestAccessWithPassphrase: success");
    napi_resolve_deferred(env, work_data->deferred, access_external);
    
cleanup:
    free(work_data->satellite_address);
    free(work_data->api_key);
    free(work_data->passphrase);
    free(work_data->user_agent);
    free(work_data->temp_directory);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

void access_satellite_complete(napi_env env, napi_status status, void* data) {
    AccessStringData* work_data = (AccessStringData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "accessSatelliteAddress");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("accessSatelliteAddress: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    {
        napi_value result_string = create_string(env, work_data->result.string);
        LOG_INFO("accessSatelliteAddress: success - %s", work_data->result.string);
        uplink_free_string_result(work_data->result);
        napi_resolve_deferred(env, work_data->deferred, result_string);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

void access_serialize_complete(napi_env env, napi_status status, void* data) {
    AccessStringData* work_data = (AccessStringData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "accessSerialize");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("accessSerialize: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    {
        napi_value result_string = create_string(env, work_data->result.string);
        LOG_INFO("accessSerialize: success");
        uplink_free_string_result(work_data->result);
        napi_resolve_deferred(env, work_data->deferred, result_string);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

void access_share_complete(napi_env env, napi_status status, void* data) {
    AccessShareData* work_data = (AccessShareData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "accessShare");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("accessShare: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    {
        napi_value access_external = create_handle_external(
            env,
            work_data->result.access->_handle,
            HANDLE_TYPE_ACCESS,
            work_data->result.access,
            NULL
        );
        
        LOG_INFO("accessShare: success, new handle=%zu", work_data->result.access->_handle);
        napi_resolve_deferred(env, work_data->deferred, access_external);
    }
    
cleanup:
    /* Free malloc'd strings stored directly in the prefixes array */
    for (size_t i = 0; i < work_data->prefix_count; i++) {
        free((char*)work_data->prefixes[i].bucket);
        free((char*)work_data->prefixes[i].prefix);
    }
    free(work_data->prefixes);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

void override_encryption_complete(napi_env env, napi_status status, void* data) {
    OverrideEncryptionData* work_data = (OverrideEncryptionData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "accessOverrideEncryptionKey");
    
    if (work_data->error != NULL) {
        LOG_ERROR("accessOverrideEncryptionKey: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    {
        LOG_INFO("accessOverrideEncryptionKey: success");
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_resolve_deferred(env, work_data->deferred, undefined);
    }
    
cleanup:
    free(work_data->bucket);
    free(work_data->prefix);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
