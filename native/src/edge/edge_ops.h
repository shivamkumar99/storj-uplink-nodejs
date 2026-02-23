/**
 * @file edge_ops.h
 * @brief Edge credential operations for uplink-nodejs
 * 
 * Declares N-API functions for edge/linkshare operations:
 * - napi_edge_register_access - Get S3 credentials from Storj edge services
 * - napi_edge_join_share_url - Create a shareable linkshare URL
 */

#ifndef EDGE_OPS_H
#define EDGE_OPS_H

#include <node_api.h>

/**
 * @brief Register access with Storj edge services to get S3-compatible credentials
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: config object { authServiceAddress, certificatePem?, insecureUnencryptedConnection? }
 *   - argv[1]: access handle (external)
 *   - argv[2]: options object (optional) { isPublic: boolean }
 * @return Promise resolving to EdgeCredentials { accessKeyId, secretKey, endpoint }
 */
napi_value napi_edge_register_access(napi_env env, napi_callback_info info);

/**
 * @brief Create a shareable linkshare URL
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: baseUrl (string) - linkshare service URL (e.g., https://link.us1.storjshare.io)
 *   - argv[1]: accessKeyId (string) - from edge_register_access, must be public
 *   - argv[2]: bucket (string, optional) - bucket name, empty shares entire project
 *   - argv[3]: key (string, optional) - object key or prefix, empty shares entire bucket
 *   - argv[4]: options (object, optional) - { raw: boolean } - raw=true serves file directly
 * @return Promise resolving to share URL string
 */
napi_value napi_edge_join_share_url(napi_env env, napi_callback_info info);

#endif /* EDGE_OPS_H */
