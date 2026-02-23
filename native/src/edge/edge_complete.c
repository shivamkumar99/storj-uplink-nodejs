/**
 * @file edge_complete.c
 * @brief Complete functions for edge async operations
 * 
 * These functions run on the main thread after execute completes.
 * They handle:
 * - Error checking and rejection
 * - Result conversion to JavaScript values
 * - Memory cleanup
 * - Promise resolution/rejection
 */

#include "edge_complete.h"
#include "edge_types.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== Complete Functions (Main Thread) ========== */

void register_access_complete(napi_env env, napi_status status, void* data) {
    RegisterAccessData* work_data = (RegisterAccessData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "edgeRegisterAccess");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("edgeRegisterAccess: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        edge_free_credentials_result(work_data->result);
        goto cleanup;
    }
    
    /* Create credentials object */
    napi_value creds_obj;
    napi_create_object(env, &creds_obj);
    
    EdgeCredentials* creds = work_data->result.credentials;
    
    napi_value access_key_id;
    napi_create_string_utf8(env, creds->access_key_id ? creds->access_key_id : "", NAPI_AUTO_LENGTH, &access_key_id);
    napi_set_named_property(env, creds_obj, "accessKeyId", access_key_id);
    
    napi_value secret_key;
    napi_create_string_utf8(env, creds->secret_key ? creds->secret_key : "", NAPI_AUTO_LENGTH, &secret_key);
    napi_set_named_property(env, creds_obj, "secretKey", secret_key);
    
    napi_value endpoint;
    napi_create_string_utf8(env, creds->endpoint ? creds->endpoint : "", NAPI_AUTO_LENGTH, &endpoint);
    napi_set_named_property(env, creds_obj, "endpoint", endpoint);
    
    LOG_INFO("edgeRegisterAccess: got credentials, accessKeyId=%s", 
             creds->access_key_id ? creds->access_key_id : "");
    
    edge_free_credentials_result(work_data->result);
    napi_resolve_deferred(env, work_data->deferred, creds_obj);
    
cleanup:
    free(work_data->auth_service_address);
    free(work_data->certificate_pem);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

void join_share_url_complete(napi_env env, napi_status status, void* data) {
    JoinShareUrlData* work_data = (JoinShareUrlData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "edgeJoinShareUrl");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("edgeJoinShareUrl: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value url;
    napi_create_string_utf8(env, work_data->result.string ? work_data->result.string : "", NAPI_AUTO_LENGTH, &url);
    
    LOG_INFO("edgeJoinShareUrl: created URL=%s", work_data->result.string ? work_data->result.string : "");
    
    /* Free the string from the result */
    if (work_data->result.string != NULL) {
        free(work_data->result.string);
    }
    
    napi_resolve_deferred(env, work_data->deferred, url);
    
cleanup:
    free(work_data->base_url);
    free(work_data->access_key_id);
    free(work_data->bucket);
    free(work_data->key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
