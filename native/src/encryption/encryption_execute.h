/**
 * @file encryption_execute.h
 * @brief Execute function declarations for encryption async operations
 * 
 * These functions run on the worker thread and call uplink-c library functions.
 */

#ifndef UPLINK_ENCRYPTION_EXECUTE_H
#define UPLINK_ENCRYPTION_EXECUTE_H

#include <node_api.h>

/**
 * @brief Execute deriveEncryptionKey on worker thread
 */
void derive_key_execute(napi_env env, void* data);

#endif /* UPLINK_ENCRYPTION_EXECUTE_H */
