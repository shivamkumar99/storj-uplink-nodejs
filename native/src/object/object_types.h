/**
 * @file object_types.h
 * @brief Data structures for object async operations
 * 
 * Contains all async work data structures used by object operations.
 */

#ifndef OBJECT_TYPES_H
#define OBJECT_TYPES_H

#include <node_api.h>
#include <stdbool.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT

/* Include uplink-c header - contains all type definitions */
#include "uplink.h"

/**
 * @brief Data for stat_object and delete_object async operations
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    UplinkObjectResult result;
    napi_deferred deferred;
    napi_async_work work;
} ObjectOpData;

/**
 * @brief Data for creating an object iterator
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* prefix;
    char* cursor;
    bool recursive;
    bool include_system;
    bool include_custom;
    size_t iterator_handle;
    napi_deferred deferred;
    napi_async_work work;
} ListObjectsCreateData;

/**
 * @brief Data for object iterator next
 */
typedef struct {
    size_t iterator_handle;
    _Bool has_next;
    napi_deferred deferred;
    napi_async_work work;
} ObjectIteratorNextData;

/**
 * @brief Data for object iterator item
 */
typedef struct {
    size_t iterator_handle;
    UplinkObject* object;
    napi_deferred deferred;
    napi_async_work work;
} ObjectIteratorItemData;

/**
 * @brief Data for object iterator error check
 */
typedef struct {
    size_t iterator_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} ObjectIteratorErrData;

/**
 * @brief Data for freeing an object iterator
 */
typedef struct {
    size_t iterator_handle;
    napi_deferred deferred;
    napi_async_work work;
} FreeObjectIteratorData;

/**
 * @brief Data for copy_object and move_object async operations
 */
typedef struct {
    size_t project_handle;
    char* src_bucket;
    char* src_key;
    char* dst_bucket;
    char* dst_key;
    UplinkObjectResult result;
    UplinkError* move_error;  /* For move operation which returns UplinkError* */
    napi_deferred deferred;
    napi_async_work work;
} CopyMoveObjectData;

/**
 * @brief Data for update_object_metadata async operation
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    UplinkCustomMetadataEntry* metadata_entries;
    size_t metadata_count;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} UpdateMetadataData;

#endif /* OBJECT_TYPES_H */
