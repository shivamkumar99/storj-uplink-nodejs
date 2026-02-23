/**
 * @file upload_complete.h
 * @brief Complete function declarations for upload async operations
 */

#ifndef UPLOAD_COMPLETE_H
#define UPLOAD_COMPLETE_H

#include <node_api.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* Complete functions - run on main thread */
void upload_object_complete(napi_env env, napi_status status, void* data);
void upload_write_complete(napi_env env, napi_status status, void* data);
void upload_commit_complete(napi_env env, napi_status status, void* data);
void upload_abort_complete(napi_env env, napi_status status, void* data);
void upload_set_metadata_complete(napi_env env, napi_status status, void* data);
void upload_info_complete(napi_env env, napi_status status, void* data);

#endif /* UPLOAD_COMPLETE_H */
