/**
 * @file upload_types.h
 * @brief Data structure definitions for upload async operations
 */

#ifndef UPLOAD_TYPES_H
#define UPLOAD_TYPES_H

#include <node_api.h>
#include <stddef.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* ========== Async Work Data Structures ========== */

/**
 * Data structure for upload_object operation
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    int64_t expires;
    UplinkUploadResult result;
    napi_deferred deferred;
    napi_async_work work;
} UploadObjectData;

/**
 * Data structure for upload_write operation
 */
typedef struct {
    size_t upload_handle;
    void* buffer_ptr;       /* Direct pointer to JS buffer (no copy) */
    size_t data_length;
    napi_ref buffer_ref;    /* Reference to keep JS buffer alive during async work */
    UplinkWriteResult result;
    napi_deferred deferred;
    napi_async_work work;
} UploadWriteData;

/**
 * Data structure for upload_commit and upload_abort operations
 */
typedef struct {
    size_t upload_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} UploadFinalizeData;

/**
 * Data structure for upload_set_custom_metadata operation
 */
typedef struct {
    size_t upload_handle;
    UplinkCustomMetadata metadata;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} UploadMetadataData;

/**
 * Data structure for upload_info operation
 */
typedef struct {
    size_t upload_handle;
    UplinkObjectResult result;
    napi_deferred deferred;
    napi_async_work work;
} UploadInfoData;

#endif /* UPLOAD_TYPES_H */
