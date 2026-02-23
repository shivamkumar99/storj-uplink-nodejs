/**
 * @file access_ops.h
 * @brief Access operations for uplink-nodejs
 * 
 * Provides N-API bindings for uplink-c access operations.
 */

#ifndef UPLINK_ACCESS_OPS_H
#define UPLINK_ACCESS_OPS_H

#include <node_api.h>

/**
 * Parse an access grant string
 * JS: parseAccess(accessGrant: string) -> Promise<AccessHandle>
 */
napi_value parse_access(napi_env env, napi_callback_info info);

/**
 * Request access with passphrase
 * JS: requestAccessWithPassphrase(satellite, apiKey, passphrase) -> Promise<AccessHandle>
 */
napi_value request_access_with_passphrase(napi_env env, napi_callback_info info);

/**
 * Request access with passphrase and config
 * JS: configRequestAccessWithPassphrase(config, satellite, apiKey, passphrase) -> Promise<AccessHandle>
 */
napi_value config_request_access_with_passphrase(napi_env env, napi_callback_info info);

/**
 * Get satellite address from access
 * JS: accessSatelliteAddress(access: AccessHandle) -> Promise<string>
 */
napi_value access_satellite_address(napi_env env, napi_callback_info info);

/**
 * Serialize access to string
 * JS: accessSerialize(access: AccessHandle) -> Promise<string>
 */
napi_value access_serialize(napi_env env, napi_callback_info info);

/**
 * Share access with restrictions
 * JS: accessShare(access, permission, prefixes) -> Promise<AccessHandle>
 */
napi_value access_share(napi_env env, napi_callback_info info);

/**
 * Override encryption key for a bucket/prefix
 * JS: accessOverrideEncryptionKey(access, bucket, prefix, encryptionKey) -> Promise<void>
 */
napi_value access_override_encryption_key(napi_env env, napi_callback_info info);

#endif /* UPLINK_ACCESS_OPS_H */
