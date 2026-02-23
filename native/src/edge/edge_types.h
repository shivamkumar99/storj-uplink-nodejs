/**
 * @file edge_types.h
 * @brief Data structures for edge async operations
 * 
 * Contains all async work data structures used by edge operations.
 * These structures hold input parameters, results, and N-API handles
 * needed for async work execution.
 */

#ifndef UPLINK_EDGE_TYPES_H
#define UPLINK_EDGE_TYPES_H

#include <node_api.h>
#include <stddef.h>
#include <stdbool.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* ========== Async Work Data Structures ========== */

/**
 * @brief Data for edgeRegisterAccess async operation
 */
typedef struct {
    char* auth_service_address;
    char* certificate_pem;
    bool insecure_unencrypted_connection;
    size_t access_handle;
    bool is_public;
    EdgeCredentialsResult result;
    napi_deferred deferred;
    napi_async_work work;
} RegisterAccessData;

/**
 * @brief Data for edgeJoinShareUrl async operation
 */
typedef struct {
    char* base_url;
    char* access_key_id;
    char* bucket;
    char* key;
    bool raw;
    UplinkStringResult result;
    napi_deferred deferred;
    napi_async_work work;
} JoinShareUrlData;

#endif /* UPLINK_EDGE_TYPES_H */
