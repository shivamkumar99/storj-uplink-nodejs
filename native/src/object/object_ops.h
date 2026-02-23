/**
 * @file object_ops.h
 * @brief Object operations header for uplink-nodejs
 * 
 * Declares N-API bindings for uplink-c object operations.
 */

#ifndef UPLINK_OBJECT_OPS_H
#define UPLINK_OBJECT_OPS_H

#include <node_api.h>

/**
 * Get object information/metadata
 * JS: statObject(projectHandle, bucket, key) -> Promise<ObjectInfo>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucket, key]
 * @return Promise resolving to ObjectInfo object
 */
napi_value stat_object(napi_env env, napi_callback_info info);

/**
 * Delete an object
 * JS: deleteObject(projectHandle, bucket, key) -> Promise<void>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucket, key]
 * @return Promise resolving to undefined
 */
napi_value delete_object(napi_env env, napi_callback_info info);

/**
 * Create an object iterator
 * JS: listObjectsCreate(projectHandle, bucket, options?) -> Promise<iteratorHandle>
 */
napi_value list_objects_create(napi_env env, napi_callback_info info);

/**
 * Advance object iterator
 * JS: objectIteratorNext(iteratorHandle) -> Promise<boolean>
 */
napi_value object_iterator_next(napi_env env, napi_callback_info info);

/**
 * Get current item from object iterator
 * JS: objectIteratorItem(iteratorHandle) -> Promise<ObjectInfo>
 */
napi_value object_iterator_item(napi_env env, napi_callback_info info);

/**
 * Check for object iterator error
 * JS: objectIteratorErr(iteratorHandle) -> Promise<null | Error>
 */
napi_value object_iterator_err(napi_env env, napi_callback_info info);

/**
 * Free an object iterator
 * JS: freeObjectIterator(iteratorHandle) -> Promise<void>
 */
napi_value free_object_iterator(napi_env env, napi_callback_info info);

/**
 * Copy an object to a new location
 * JS: copyObject(projectHandle, srcBucket, srcKey, dstBucket, dstKey) -> Promise<ObjectInfo>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, srcBucket, srcKey, dstBucket, dstKey]
 * @return Promise resolving to ObjectInfo of the new object
 */
napi_value copy_object(napi_env env, napi_callback_info info);

/**
 * Move/rename an object
 * JS: moveObject(projectHandle, srcBucket, srcKey, dstBucket, dstKey) -> Promise<void>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, srcBucket, srcKey, dstBucket, dstKey]
 * @return Promise resolving to undefined
 */
napi_value move_object(napi_env env, napi_callback_info info);

/**
 * Update object custom metadata
 * JS: updateObjectMetadata(projectHandle, bucket, key, metadata) -> Promise<void>
 * 
 * @param env N-API environment
 * @param info Callback info containing [projectHandle, bucket, key, metadata]
 *             metadata is an object with string key-value pairs
 * @return Promise resolving to undefined
 */
napi_value update_object_metadata(napi_env env, napi_callback_info info);

#endif /* UPLINK_OBJECT_OPS_H */
