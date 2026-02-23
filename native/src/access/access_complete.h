/**
 * @file access_complete.h
 * @brief Complete function declarations for access async operations
 * 
 * These functions run on the main thread after execute completes.
 * They handle result conversion to JavaScript and cleanup.
 */

#ifndef UPLINK_ACCESS_COMPLETE_H
#define UPLINK_ACCESS_COMPLETE_H

#include <node_api.h>

/**
 * @brief Complete parseAccess on main thread
 */
void parse_access_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete requestAccessWithPassphrase on main thread
 */
void request_access_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete configRequestAccessWithPassphrase on main thread
 */
void config_request_access_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete accessSatelliteAddress on main thread
 */
void access_satellite_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete accessSerialize on main thread
 */
void access_serialize_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete accessShare on main thread
 */
void access_share_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete accessOverrideEncryptionKey on main thread
 */
void override_encryption_complete(napi_env env, napi_status status, void* data);

#endif /* UPLINK_ACCESS_COMPLETE_H */
