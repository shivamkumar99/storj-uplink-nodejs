/**
 * @file access_types.h
 * @brief Data structures for access async operations
 * 
 * Contains all async work data structures used by access operations.
 * These structures hold input parameters, results, and N-API handles
 * needed for async work execution.
 */

#ifndef UPLINK_ACCESS_TYPES_H
#define UPLINK_ACCESS_TYPES_H

#include <node_api.h>
#include <stddef.h>
#include <stdint.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* ========== Async Work Data Structures ========== */

/**
 * @brief Data for parseAccess async operation
 */
typedef struct {
    char* access_grant;
    UplinkAccessResult result;
    napi_deferred deferred;
    napi_async_work work;
} ParseAccessData;

/**
 * @brief Data for requestAccessWithPassphrase async operation
 */
typedef struct {
    char* satellite_address;
    char* api_key;
    char* passphrase;
    UplinkAccessResult result;
    napi_deferred deferred;
    napi_async_work work;
} RequestAccessData;

/**
 * @brief Data for configRequestAccessWithPassphrase async operation
 */
typedef struct {
    char* satellite_address;
    char* api_key;
    char* passphrase;
    char* user_agent;
    int32_t dial_timeout_milliseconds;
    char* temp_directory;
    UplinkAccessResult result;
    napi_deferred deferred;
    napi_async_work work;
} ConfigRequestAccessData;

/**
 * @brief Data for accessSatelliteAddress and accessSerialize async operations
 */
typedef struct {
    size_t access_handle;
    UplinkStringResult result;
    napi_deferred deferred;
    napi_async_work work;
} AccessStringData;

/**
 * @brief Data for accessShare async operation
 */
typedef struct {
    size_t access_handle;
    UplinkPermission permission;
    UplinkSharePrefix* prefixes;
    size_t prefix_count;
    UplinkAccessResult result;
    napi_deferred deferred;
    napi_async_work work;
} AccessShareData;

/**
 * @brief Data for accessOverrideEncryptionKey async operation
 */
typedef struct {
    size_t access_handle;
    char* bucket;
    char* prefix;
    size_t encryption_key_handle;
    UplinkError* error;
    napi_deferred deferred;
    napi_async_work work;
} OverrideEncryptionData;

#endif /* UPLINK_ACCESS_TYPES_H */
