/**
 * @file multipart_complete.h
 * @brief Complete function declarations for multipart upload operations
 * 
 * These functions run on the main thread after the worker thread completes.
 * They handle error checking, result conversion to JavaScript, and cleanup.
 */

#ifndef MULTIPART_COMPLETE_H
#define MULTIPART_COMPLETE_H

#include <node_api.h>

/**
 * @brief Complete begin_upload on main thread
 */
void begin_upload_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete commit_upload on main thread
 */
void commit_upload_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete abort_upload on main thread
 */
void abort_upload_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete upload_part on main thread
 */
void upload_part_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete part_upload_write on main thread
 */
void part_upload_write_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete part_upload_commit on main thread
 */
void part_upload_commit_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete part_upload_abort on main thread
 */
void part_upload_abort_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete part_upload_set_etag on main thread
 */
void part_upload_set_etag_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete part_upload_info on main thread
 */
void part_upload_info_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete list_upload_parts_create on main thread
 */
void list_upload_parts_create_complete(napi_env env, napi_status status, void* data);
void part_iterator_next_complete(napi_env env, napi_status status, void* data);
void part_iterator_item_complete(napi_env env, napi_status status, void* data);
void part_iterator_err_complete(napi_env env, napi_status status, void* data);
void free_part_iterator_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete list_uploads_create on main thread
 */
void list_uploads_create_complete(napi_env env, napi_status status, void* data);
void upload_iterator_next_complete(napi_env env, napi_status status, void* data);
void upload_iterator_item_complete(napi_env env, napi_status status, void* data);
void upload_iterator_err_complete(napi_env env, napi_status status, void* data);
void free_upload_iterator_complete(napi_env env, napi_status status, void* data);

#endif /* MULTIPART_COMPLETE_H */
