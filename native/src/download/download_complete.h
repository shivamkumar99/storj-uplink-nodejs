/**
 * @file download_complete.h
 * @brief Complete function declarations for download async operations
 */

#ifndef DOWNLOAD_COMPLETE_H
#define DOWNLOAD_COMPLETE_H

#include <node_api.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* Complete functions - run on main thread */
void download_object_complete(napi_env env, napi_status status, void* data);
void download_read_complete(napi_env env, napi_status status, void* data);
void download_info_complete(napi_env env, napi_status status, void* data);
void close_download_complete(napi_env env, napi_status status, void* data);

#endif /* DOWNLOAD_COMPLETE_H */
