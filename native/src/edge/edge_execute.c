/**
 * @file edge_execute.c
 * @brief Execute functions for edge async operations
 * 
 * These functions run on the worker thread and call uplink-c library functions.
 * They should NOT call any N-API functions (except env is passed but unused).
 */

#include "edge_execute.h"
#include "edge_types.h"
#include "../common/logger.h"

#include <string.h>

/* ========== Execute Functions (Worker Thread) ========== */

void register_access_execute(napi_env env, void* data) {
    (void)env;
    RegisterAccessData* work_data = (RegisterAccessData*)data;
    
    LOG_DEBUG("edgeRegisterAccess: registering access (worker thread)");
    
    EdgeConfig config = {0};
    config.auth_service_address = work_data->auth_service_address;
    config.certificate_pem = work_data->certificate_pem;
    config.insecure_unencrypted_connection = work_data->insecure_unencrypted_connection;
    
    UplinkAccess access = { ._handle = work_data->access_handle };
    
    EdgeRegisterAccessOptions opts = {0};
    opts.is_public = work_data->is_public;
    
    work_data->result = edge_register_access(config, &access, &opts);
}

void join_share_url_execute(napi_env env, void* data) {
    (void)env;
    JoinShareUrlData* work_data = (JoinShareUrlData*)data;
    
    LOG_DEBUG("edgeJoinShareUrl: joining share URL (worker thread)");
    
    EdgeShareURLOptions* opts = NULL;
    EdgeShareURLOptions options = {0};
    if (work_data->raw) {
        options.raw = true;
        opts = &options;
    }
    
    work_data->result = edge_join_share_url(
        work_data->base_url,
        work_data->access_key_id,
        work_data->bucket,
        work_data->key,
        opts
    );
}
