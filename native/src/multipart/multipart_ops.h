/**
 * @file multipart_ops.h
 * @brief Multipart upload operations for uplink-nodejs
 * 
 * Declares N-API functions for multipart upload operations:
 * - begin_upload - Start a multipart upload
 * - commit_upload - Complete a multipart upload
 * - abort_upload - Cancel a multipart upload
 * - upload_part - Start uploading a part
 * - part_upload_write - Write data to a part
 * - part_upload_commit - Commit a part
 * - part_upload_abort - Abort a part
 * - part_upload_set_etag - Set part ETag
 * - part_upload_info - Get part info
 * - list_upload_parts - List uploaded parts
 * - list_uploads - List pending multipart uploads
 */

#ifndef MULTIPART_OPS_H
#define MULTIPART_OPS_H

#include <node_api.h>

/**
 * @brief Begin a multipart upload
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: project handle (external)
 *   - argv[1]: bucket name (string)
 *   - argv[2]: object key (string)
 *   - argv[3]: options (object, optional) - { expires: number }
 * @return Promise resolving to UploadInfo { uploadId, key, isPrefix, system, custom }
 */
napi_value begin_upload(napi_env env, napi_callback_info info);

/**
 * @brief Commit (complete) a multipart upload
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: project handle (external)
 *   - argv[1]: bucket name (string)
 *   - argv[2]: object key (string)
 *   - argv[3]: upload ID (string)
 *   - argv[4]: options (object, optional) - { customMetadata: object }
 * @return Promise resolving to ObjectInfo
 */
napi_value commit_upload(napi_env env, napi_callback_info info);

/**
 * @brief Abort a multipart upload
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: project handle (external)
 *   - argv[1]: bucket name (string)
 *   - argv[2]: object key (string)
 *   - argv[3]: upload ID (string)
 * @return Promise resolving to void
 */
napi_value abort_upload(napi_env env, napi_callback_info info);

/**
 * @brief Start uploading a part
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: project handle (external)
 *   - argv[1]: bucket name (string)
 *   - argv[2]: object key (string)
 *   - argv[3]: upload ID (string)
 *   - argv[4]: part number (number, 1-based)
 * @return Promise resolving to part upload handle
 */
napi_value upload_part(napi_env env, napi_callback_info info);

/**
 * @brief Write data to a part
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: part upload handle (external)
 *   - argv[1]: data buffer (Buffer)
 *   - argv[2]: length (number)
 * @return Promise resolving to number of bytes written
 */
napi_value part_upload_write(napi_env env, napi_callback_info info);

/**
 * @brief Commit a part
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: part upload handle (external)
 * @return Promise resolving to void
 */
napi_value part_upload_commit(napi_env env, napi_callback_info info);

/**
 * @brief Abort a part upload
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: part upload handle (external)
 * @return Promise resolving to void
 */
napi_value part_upload_abort(napi_env env, napi_callback_info info);

/**
 * @brief Set part ETag
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: part upload handle (external)
 *   - argv[1]: etag (string)
 * @return Promise resolving to void
 */
napi_value part_upload_set_etag(napi_env env, napi_callback_info info);

/**
 * @brief Get part upload info
 * @param env N-API environment
 * @param info Callback info containing:
 *   - argv[0]: part upload handle (external)
 * @return Promise resolving to PartInfo { partNumber, size, modified, etag }
 */
napi_value part_upload_info(napi_env env, napi_callback_info info);

/**
 * @brief Create a part iterator for a multipart upload
 */
napi_value list_upload_parts_create(napi_env env, napi_callback_info info);

/** @brief Advance part iterator */
napi_value part_iterator_next(napi_env env, napi_callback_info info);

/** @brief Get current part from iterator */
napi_value part_iterator_item(napi_env env, napi_callback_info info);

/** @brief Check for part iterator error */
napi_value part_iterator_err(napi_env env, napi_callback_info info);

/** @brief Free a part iterator */
napi_value free_part_iterator(napi_env env, napi_callback_info info);

/**
 * @brief Create an upload iterator for listing pending uploads
 */
napi_value list_uploads_create(napi_env env, napi_callback_info info);

/** @brief Advance upload iterator */
napi_value upload_iterator_next(napi_env env, napi_callback_info info);

/** @brief Get current upload info from iterator */
napi_value upload_iterator_item(napi_env env, napi_callback_info info);

/** @brief Check for upload iterator error */
napi_value upload_iterator_err(napi_env env, napi_callback_info info);

/** @brief Free an upload iterator */
napi_value free_upload_iterator(napi_env env, napi_callback_info info);

#endif /* MULTIPART_OPS_H */
