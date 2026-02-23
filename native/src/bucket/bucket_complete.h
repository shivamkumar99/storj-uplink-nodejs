/**
 * @file bucket_complete.h
 * @brief Complete function declarations for bucket async operations
 */

#ifndef BUCKET_COMPLETE_H
#define BUCKET_COMPLETE_H

#include <node_api.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* Helper function to convert bucket to JS object */
napi_value uplink_bucket_to_js(napi_env env, UplinkBucket* bucket);

/* Complete functions - run on main thread */
void create_bucket_complete(napi_env env, napi_status status, void* data);
void ensure_bucket_complete(napi_env env, napi_status status, void* data);
void stat_bucket_complete(napi_env env, napi_status status, void* data);
void delete_bucket_complete(napi_env env, napi_status status, void* data);
void delete_bucket_with_objects_complete(napi_env env, napi_status status, void* data);
void list_buckets_create_complete(napi_env env, napi_status status, void* data);
void bucket_iterator_next_complete(napi_env env, napi_status status, void* data);
void bucket_iterator_item_complete(napi_env env, napi_status status, void* data);
void bucket_iterator_err_complete(napi_env env, napi_status status, void* data);
void free_bucket_iterator_complete(napi_env env, napi_status status, void* data);

#endif /* BUCKET_COMPLETE_H */
