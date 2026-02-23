/**
 * @file access_execute.c
 * @brief Execute functions for access async operations
 * 
 * These functions run on the worker thread and call uplink-c library functions.
 * They should NOT call any N-API functions (except env is passed but unused).
 */

#include "access_execute.h"
#include "access_types.h"
#include "../common/logger.h"

/* ========== Execute Functions (Worker Thread) ========== */

void parse_access_execute(napi_env env, void* data) {
    (void)env;
    ParseAccessData* work_data = (ParseAccessData*)data;
    
    LOG_DEBUG("parseAccess: parsing access grant (worker thread)");
    work_data->result = uplink_parse_access(work_data->access_grant);
}

void request_access_execute(napi_env env, void* data) {
    (void)env;
    RequestAccessData* work_data = (RequestAccessData*)data;
    
    LOG_DEBUG("requestAccessWithPassphrase: requesting access (worker thread)");
    work_data->result = uplink_request_access_with_passphrase(
        work_data->satellite_address,
        work_data->api_key,
        work_data->passphrase
    );
}

void config_request_access_execute(napi_env env, void* data) {
    (void)env;
    ConfigRequestAccessData* work_data = (ConfigRequestAccessData*)data;
    
    LOG_DEBUG("configRequestAccessWithPassphrase: requesting access (worker thread)");
    
    UplinkConfig config = {
        .user_agent = work_data->user_agent,
        .dial_timeout_milliseconds = work_data->dial_timeout_milliseconds,
        .temp_directory = work_data->temp_directory
    };
    
    work_data->result = uplink_config_request_access_with_passphrase(
        config,
        work_data->satellite_address,
        work_data->api_key,
        work_data->passphrase
    );
}

void access_satellite_execute(napi_env env, void* data) {
    (void)env;
    AccessStringData* work_data = (AccessStringData*)data;
    
    LOG_DEBUG("accessSatelliteAddress: getting address (worker thread)");
    
    UplinkAccess access = { ._handle = work_data->access_handle };
    work_data->result = uplink_access_satellite_address(&access);
}

void access_serialize_execute(napi_env env, void* data) {
    (void)env;
    AccessStringData* work_data = (AccessStringData*)data;
    
    LOG_DEBUG("accessSerialize: serializing access (worker thread)");
    
    UplinkAccess access = { ._handle = work_data->access_handle };
    work_data->result = uplink_access_serialize(&access);
}

void access_share_execute(napi_env env, void* data) {
    (void)env;
    AccessShareData* work_data = (AccessShareData*)data;
    
    LOG_DEBUG("accessShare: sharing access (worker thread)");
    
    UplinkAccess access = { ._handle = work_data->access_handle };
    work_data->result = uplink_access_share(
        &access,
        work_data->permission,
        work_data->prefixes,
        (int)work_data->prefix_count
    );
}

void override_encryption_execute(napi_env env, void* data) {
    (void)env;
    OverrideEncryptionData* work_data = (OverrideEncryptionData*)data;
    
    LOG_DEBUG("accessOverrideEncryptionKey: overriding key (worker thread)");
    
    UplinkAccess access = { ._handle = work_data->access_handle };
    UplinkEncryptionKey key = { ._handle = work_data->encryption_key_handle };
    
    work_data->error = uplink_access_override_encryption_key(
        &access,
        work_data->bucket,
        work_data->prefix,
        &key
    );
}
