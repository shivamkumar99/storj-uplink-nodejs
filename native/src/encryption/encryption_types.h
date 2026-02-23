/**
 * @file encryption_types.h
 * @brief Data structures for encryption async operations
 * 
 * Contains all async work data structures used by encryption operations.
 * These structures hold input parameters, results, and N-API handles
 * needed for async work execution.
 */

#ifndef UPLINK_ENCRYPTION_TYPES_H
#define UPLINK_ENCRYPTION_TYPES_H

#include <node_api.h>
#include <stddef.h>

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

/* ========== Async Work Data Structures ========== */

/**
 * @brief Data for deriveEncryptionKey async operation
 */
typedef struct {
    char* passphrase;
    void* salt;
    size_t salt_length;
    UplinkEncryptionKeyResult result;
    napi_deferred deferred;
    napi_async_work work;
} DeriveKeyData;

#endif /* UPLINK_ENCRYPTION_TYPES_H */
