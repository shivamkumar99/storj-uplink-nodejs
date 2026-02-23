/**
 * @file bucket_types.h
 * @brief Data structure definitions for bucket async operations
 */

#ifndef BUCKET_TYPES_H
#define BUCKET_TYPES_H

#include <node_api.h>
#include <stddef.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* ========== Async Work Data Structures ========== */

/**
 * Data structure for single bucket operations
 * Used by: create_bucket, ensure_bucket, stat_bucket, delete_bucket, delete_bucket_with_objects
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    UplinkBucketResult result;
    napi_deferred deferred;
    napi_async_work work;
} BucketOpData;

/**
 * Data structure for creating a bucket iterator
 * Used by: list_buckets_create
 */
typedef struct {
    size_t project_handle;
    char* cursor;
    size_t iterator_handle;
    napi_deferred deferred;
    napi_async_work work;
} ListBucketsCreateData;

/**
 * Data structure for bucket iterator next
 * Used by: bucket_iterator_next
 */
typedef struct {
    size_t iterator_handle;
    _Bool has_next;
    napi_deferred deferred;
    napi_async_work work;
} BucketIteratorNextData;

/**
 * Data structure for bucket iterator item
 * Used by: bucket_iterator_item
 */
typedef struct {
    size_t iterator_handle;
    UplinkBucket* bucket;
    napi_deferred deferred;
    napi_async_work work;
} BucketIteratorItemData;

/**
 * Data structure for bucket iterator error check
 * Used by: bucket_iterator_err
 */
typedef struct {
    size_t iterator_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} BucketIteratorErrData;

/**
 * Data structure for freeing a bucket iterator
 * Used by: free_bucket_iterator
 */
typedef struct {
    size_t iterator_handle;
    napi_deferred deferred;
    napi_async_work work;
} FreeBucketIteratorData;

#endif /* BUCKET_TYPES_H */
