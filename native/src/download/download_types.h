/**
 * @file download_types.h
 * @brief Data structure definitions for download async operations
 */

#ifndef DOWNLOAD_TYPES_H
#define DOWNLOAD_TYPES_H

#include <node_api.h>
#include <stddef.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* ========== Async Work Data Structures ========== */

/**
 * Data structure for download_object operation
 */
typedef struct {
    size_t project_handle;
    char* bucket_name;
    char* object_key;
    int64_t offset;
    int64_t length;
    UplinkDownloadResult result;
    napi_deferred deferred;
    napi_async_work work;
} DownloadObjectData;

/**
 * Data structure for download_read operation
 */
typedef struct {
    size_t download_handle;
    void* buffer_ptr;       /* Direct pointer to JS buffer (no copy) */
    size_t data_length;
    napi_ref buffer_ref;    /* Reference to keep JS buffer alive during async work */
    UplinkReadResult result;
    napi_deferred deferred;
    napi_async_work work;
} DownloadReadData;

/**
 * Data structure for download_info operation
 */
typedef struct {
    size_t download_handle;
    UplinkObjectResult result;
    napi_deferred deferred;
    napi_async_work work;
} DownloadInfoData;

/**
 * Data structure for close_download operation
 */
typedef struct {
    size_t download_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} CloseDownloadData;

#endif /* DOWNLOAD_TYPES_H */
