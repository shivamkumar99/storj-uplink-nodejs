/**
 * @file project_types.h
 * @brief Data structure definitions for project async operations
 */

#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <node_api.h>
#include <stddef.h>
#include <stdint.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* ========== Async Work Data Structures ========== */

/**
 * Data structure for open_project operation
 */
typedef struct {
    size_t access_handle;
    UplinkProjectResult result;
    napi_deferred deferred;
    napi_async_work work;
} OpenProjectData;

/**
 * Data structure for config_open_project operation
 */
typedef struct {
    size_t access_handle;
    char* user_agent;
    int32_t dial_timeout_milliseconds;
    char* temp_directory;
    UplinkProjectResult result;
    napi_deferred deferred;
    napi_async_work work;
} ConfigOpenProjectData;

/**
 * Data structure for close_project operation
 */
typedef struct {
    size_t project_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} CloseProjectData;

/**
 * Data structure for revoke_access operation
 */
typedef struct {
    size_t project_handle;
    size_t access_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} RevokeAccessData;

#endif /* PROJECT_TYPES_H */
