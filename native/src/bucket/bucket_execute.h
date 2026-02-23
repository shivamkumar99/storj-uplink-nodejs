/**
 * @file bucket_execute.h
 * @brief Execute function declarations for bucket async operations
 */

#ifndef BUCKET_EXECUTE_H
#define BUCKET_EXECUTE_H

#include <node_api.h>

/* Execute functions - run on worker thread */
void create_bucket_execute(napi_env env, void* data);
void ensure_bucket_execute(napi_env env, void* data);
void stat_bucket_execute(napi_env env, void* data);
void delete_bucket_execute(napi_env env, void* data);
void delete_bucket_with_objects_execute(napi_env env, void* data);
void list_buckets_create_execute(napi_env env, void* data);
void bucket_iterator_next_execute(napi_env env, void* data);
void bucket_iterator_item_execute(napi_env env, void* data);
void bucket_iterator_err_execute(napi_env env, void* data);
void free_bucket_iterator_execute(napi_env env, void* data);

#endif /* BUCKET_EXECUTE_H */
