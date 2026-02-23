/**
 * @file encryption_complete.h
 * @brief Complete function declarations for encryption async operations
 * 
 * These functions run on the main thread after execute completes.
 * They handle result conversion to JavaScript and cleanup.
 */

#ifndef UPLINK_ENCRYPTION_COMPLETE_H
#define UPLINK_ENCRYPTION_COMPLETE_H

#include <node_api.h>

/**
 * @brief Complete deriveEncryptionKey on main thread
 */
void derive_key_complete(napi_env env, napi_status status, void* data);

#endif /* UPLINK_ENCRYPTION_COMPLETE_H */
