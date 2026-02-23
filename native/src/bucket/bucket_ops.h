/**
 * @file bucket_ops.h
 * @brief Bucket operations header for uplink-nodejs
 * 
 * Declares N-API bindings for uplink-c bucket operations.
 */

#ifndef UPLINK_BUCKET_OPS_H
#define UPLINK_BUCKET_OPS_H

#include <node_api.h>

/**
 * Create a new bucket
 * JS: createBucket(projectHandle, name) -> Promise<BucketInfo>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucketName]
 * @return Promise resolving to BucketInfo object
 */
napi_value create_bucket(napi_env env, napi_callback_info info);

/**
 * Ensure bucket exists (create if not exists)
 * JS: ensureBucket(projectHandle, name) -> Promise<BucketInfo>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucketName]
 * @return Promise resolving to BucketInfo object
 */
napi_value ensure_bucket(napi_env env, napi_callback_info info);

/**
 * Get bucket information
 * JS: statBucket(projectHandle, name) -> Promise<BucketInfo>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucketName]
 * @return Promise resolving to BucketInfo object
 */
napi_value stat_bucket(napi_env env, napi_callback_info info);

/**
 * Delete an empty bucket
 * JS: deleteBucket(projectHandle, name) -> Promise<void>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucketName]
 * @return Promise resolving to undefined
 */
napi_value delete_bucket(napi_env env, napi_callback_info info);

/**
 * Delete a bucket and all its objects
 * JS: deleteBucketWithObjects(projectHandle, name) -> Promise<void>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucketName]
 * @return Promise resolving to undefined
 */
napi_value delete_bucket_with_objects(napi_env env, napi_callback_info info);

/**
 * Create a bucket iterator
 * JS: listBucketsCreate(projectHandle, options?) -> Promise<iteratorHandle>
 */
napi_value list_buckets_create(napi_env env, napi_callback_info info);

/**
 * Advance bucket iterator to the next item
 * JS: bucketIteratorNext(iteratorHandle) -> Promise<boolean>
 */
napi_value bucket_iterator_next(napi_env env, napi_callback_info info);

/**
 * Get current item from bucket iterator
 * JS: bucketIteratorItem(iteratorHandle) -> Promise<BucketInfo>
 */
napi_value bucket_iterator_item(napi_env env, napi_callback_info info);

/**
 * Check for iterator error after iteration completes
 * JS: bucketIteratorErr(iteratorHandle) -> Promise<null | Error>
 */
napi_value bucket_iterator_err(napi_env env, napi_callback_info info);

/**
 * Free a bucket iterator
 * JS: freeBucketIterator(iteratorHandle) -> Promise<void>
 */
napi_value free_bucket_iterator(napi_env env, napi_callback_info info);

#endif /* UPLINK_BUCKET_OPS_H */
