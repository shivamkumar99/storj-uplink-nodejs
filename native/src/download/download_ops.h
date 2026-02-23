/**
 * @file download_ops.h
 * @brief Download operations declarations for uplink-nodejs
 * 
 * Declares N-API bindings for uplink-c download operations:
 * - download_object: Start a download from a bucket
 * - download_read: Read data from a download
 * - download_info: Get info about the downloaded object
 * - close_download: Close the download stream
 */

#ifndef DOWNLOAD_OPS_H
#define DOWNLOAD_OPS_H

#include <node_api.h>

/**
 * Start downloading an object from a bucket
 * 
 * @param env N-API environment
 * @param info Callback info containing:
 *   - arg[0]: project handle (external)
 *   - arg[1]: bucket name (string)
 *   - arg[2]: object key (string)
 *   - arg[3]: options object (optional) { offset?: number, length?: number }
 * @returns Promise<{ downloadHandle: external }>
 */
napi_value download_object(napi_env env, napi_callback_info info);

/**
 * Read data from a download into a buffer
 * 
 * @param env N-API environment
 * @param info Callback info containing:
 *   - arg[0]: download handle (external)
 *   - arg[1]: buffer to read into (Buffer)
 *   - arg[2]: length to read (number)
 * @returns Promise<{ bytesRead: number }>
 */
napi_value download_read(napi_env env, napi_callback_info info);

/**
 * Get info about the downloaded object
 * 
 * @param env N-API environment
 * @param info Callback info containing:
 *   - arg[0]: download handle (external)
 * @returns Promise<ObjectInfo>
 */
napi_value download_info(napi_env env, napi_callback_info info);

/**
 * Close the download stream
 * 
 * @param env N-API environment
 * @param info Callback info containing:
 *   - arg[0]: download handle (external)
 * @returns Promise<void>
 */
napi_value close_download(napi_env env, napi_callback_info info);

#endif /* DOWNLOAD_OPS_H */
