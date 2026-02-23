/**
 * @file upload_ops.h
 * @brief Upload operations for uplink-nodejs
 * 
 * Provides native functions for upload operations.
 * All functions are async (return Promises).
 */

#ifndef UPLINK_UPLOAD_OPS_H
#define UPLINK_UPLOAD_OPS_H

#include <node_api.h>

/**
 * Start an upload
 * JS: uploadObject(project: ProjectHandle, bucket: string, key: string, options?: UploadOptions): Promise<UploadHandle>
 */
napi_value upload_object(napi_env env, napi_callback_info info);

/**
 * Write data to upload
 * JS: uploadWrite(upload: UploadHandle, data: Buffer): Promise<number>
 */
napi_value upload_write(napi_env env, napi_callback_info info);

/**
 * Commit/finalize upload
 * JS: uploadCommit(upload: UploadHandle): Promise<void>
 */
napi_value upload_commit(napi_env env, napi_callback_info info);

/**
 * Abort upload
 * JS: uploadAbort(upload: UploadHandle): Promise<void>
 */
napi_value upload_abort(napi_env env, napi_callback_info info);

/**
 * Set custom metadata on upload
 * JS: uploadSetCustomMetadata(upload: UploadHandle, metadata: Record<string, string>): Promise<void>
 */
napi_value upload_set_custom_metadata(napi_env env, napi_callback_info info);

/**
 * Get upload info
 * JS: uploadInfo(upload: UploadHandle): Promise<ObjectInfo>
 */
napi_value upload_info(napi_env env, napi_callback_info info);

#endif /* UPLINK_UPLOAD_OPS_H */
