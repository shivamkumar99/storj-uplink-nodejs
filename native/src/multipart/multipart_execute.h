/**
 * @file multipart_execute.h
 * @brief Execute function declarations for multipart upload operations
 * 
 * These functions run on the worker thread and perform the actual
 * uplink-c library calls. They should not call any N-API functions
 * except napi_env (which is unused).
 */

#ifndef MULTIPART_EXECUTE_H
#define MULTIPART_EXECUTE_H

#include <node_api.h>

/**
 * @brief Execute begin_upload on worker thread
 */
void begin_upload_execute(napi_env env, void* data);

/**
 * @brief Execute commit_upload on worker thread
 */
void commit_upload_execute(napi_env env, void* data);

/**
 * @brief Execute abort_upload on worker thread
 */
void abort_upload_execute(napi_env env, void* data);

/**
 * @brief Execute upload_part on worker thread
 */
void upload_part_execute(napi_env env, void* data);

/**
 * @brief Execute part_upload_write on worker thread
 */
void part_upload_write_execute(napi_env env, void* data);

/**
 * @brief Execute part_upload_commit on worker thread
 */
void part_upload_commit_execute(napi_env env, void* data);

/**
 * @brief Execute part_upload_abort on worker thread
 */
void part_upload_abort_execute(napi_env env, void* data);

/**
 * @brief Execute part_upload_set_etag on worker thread
 */
void part_upload_set_etag_execute(napi_env env, void* data);

/**
 * @brief Execute part_upload_info on worker thread
 */
void part_upload_info_execute(napi_env env, void* data);

/**
 * @brief Execute list_upload_parts_create on worker thread
 */
void list_upload_parts_create_execute(napi_env env, void* data);
void part_iterator_next_execute(napi_env env, void* data);
void part_iterator_item_execute(napi_env env, void* data);
void part_iterator_err_execute(napi_env env, void* data);
void free_part_iterator_execute(napi_env env, void* data);

/**
 * @brief Execute list_uploads_create on worker thread
 */
void list_uploads_create_execute(napi_env env, void* data);
void upload_iterator_next_execute(napi_env env, void* data);
void upload_iterator_item_execute(napi_env env, void* data);
void upload_iterator_err_execute(napi_env env, void* data);
void free_upload_iterator_execute(napi_env env, void* data);

#endif /* MULTIPART_EXECUTE_H */
