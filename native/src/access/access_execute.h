/**
 * @file access_execute.h
 * @brief Execute function declarations for access async operations
 * 
 * These functions run on the worker thread and call uplink-c library functions.
 */

#ifndef UPLINK_ACCESS_EXECUTE_H
#define UPLINK_ACCESS_EXECUTE_H

#include <node_api.h>

/**
 * @brief Execute parseAccess on worker thread
 */
void parse_access_execute(napi_env env, void* data);

/**
 * @brief Execute requestAccessWithPassphrase on worker thread
 */
void request_access_execute(napi_env env, void* data);

/**
 * @brief Execute configRequestAccessWithPassphrase on worker thread
 */
void config_request_access_execute(napi_env env, void* data);

/**
 * @brief Execute accessSatelliteAddress on worker thread
 */
void access_satellite_execute(napi_env env, void* data);

/**
 * @brief Execute accessSerialize on worker thread
 */
void access_serialize_execute(napi_env env, void* data);

/**
 * @brief Execute accessShare on worker thread
 */
void access_share_execute(napi_env env, void* data);

/**
 * @brief Execute accessOverrideEncryptionKey on worker thread
 */
void override_encryption_execute(napi_env env, void* data);

#endif /* UPLINK_ACCESS_EXECUTE_H */
