/**
 * @file multipart_types.h
 * @brief Data structures for multipart upload async operations
 * 
 * Contains all async work data structures used by multipart operations.
 * These structures are used to pass data between the main thread,
 * worker thread, and completion callback.
 */

#ifndef MULTIPART_TYPES_H
#define MULTIPART_TYPES_H

#include <node_api.h>
#include <stdbool.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT

/* Include uplink-c header - contains all type definitions */
#include "uplink.h"

/**
 * @brief Data for begin_upload async operation
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    int64_t expires;
    UplinkUploadInfoResult result;
    napi_deferred deferred;
    napi_async_work work;
} BeginUploadData;

/**
 * @brief Data for commit_upload async operation
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    char* upload_id;
    UplinkCustomMetadataEntry* metadata_entries;
    size_t metadata_count;
    UplinkCommitUploadResult result;
    napi_deferred deferred;
    napi_async_work work;
} CommitUploadData;

/**
 * @brief Data for abort_upload async operation
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    char* upload_id;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} AbortUploadData;

/**
 * @brief Data for upload_part async operation
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    char* upload_id;
    uint32_t part_number;
    UplinkPartUploadResult result;
    napi_deferred deferred;
    napi_async_work work;
} UploadPartData;

/**
 * @brief Data for part_upload_write async operation
 */
typedef struct {
    size_t part_upload_handle;
    void* buffer;
    size_t length;
    UplinkWriteResult result;
    napi_ref buffer_ref;
    napi_deferred deferred;
    napi_async_work work;
} PartUploadWriteData;

/**
 * @brief Data for part_upload_commit and part_upload_abort async operations
 */
typedef struct {
    size_t part_upload_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} PartUploadOpData;

/**
 * @brief Data for part_upload_set_etag async operation
 */
typedef struct {
    size_t part_upload_handle;
    char* etag;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} PartUploadSetEtagData;

/**
 * @brief Data for part_upload_info async operation
 */
typedef struct {
    size_t part_upload_handle;
    UplinkPartResult result;
    napi_deferred deferred;
    napi_async_work work;
} PartUploadInfoData;

/**
 * @brief Data for creating an upload parts iterator
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    char* upload_id;
    uint32_t cursor;
    size_t iterator_handle;
    napi_deferred deferred;
    napi_async_work work;
} ListUploadPartsCreateData;

/**
 * @brief Data for part iterator next
 */
typedef struct {
    size_t iterator_handle;
    _Bool has_next;
    napi_deferred deferred;
    napi_async_work work;
} PartIteratorNextData;

/**
 * @brief Data for part iterator item
 */
typedef struct {
    size_t iterator_handle;
    UplinkPart* part;
    napi_deferred deferred;
    napi_async_work work;
} PartIteratorItemData;

/**
 * @brief Data for part iterator error check
 */
typedef struct {
    size_t iterator_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} PartIteratorErrData;

/**
 * @brief Data for freeing a part iterator
 */
typedef struct {
    size_t iterator_handle;
    napi_deferred deferred;
    napi_async_work work;
} FreePartIteratorData;

/**
 * @brief Data for creating an uploads iterator
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
} ListUploadsCreateData;

/**
 * @brief Data for upload iterator next
 */
typedef struct {
    size_t iterator_handle;
    _Bool has_next;
    napi_deferred deferred;
    napi_async_work work;
} UploadIteratorNextData;

/**
 * @brief Data for upload iterator item
 */
typedef struct {
    size_t iterator_handle;
    UplinkUploadInfo* upload_info;
    napi_deferred deferred;
    napi_async_work work;
} UploadIteratorItemData;

/**
 * @brief Data for upload iterator error check
 */
typedef struct {
    size_t iterator_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} UploadIteratorErrData;

/**
 * @brief Data for freeing an upload iterator
 */
typedef struct {
    size_t iterator_handle;
    napi_deferred deferred;
    napi_async_work work;
} FreeUploadIteratorData;

#endif /* MULTIPART_TYPES_H */
