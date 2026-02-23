/**
 * @file encryption_ops.h
 * @brief Encryption operations declarations for uplink-nodejs
 * 
 * Declares N-API bindings for uplink-c encryption operations:
 * - derive_encryption_key: Derive key from passphrase and salt
 */

#ifndef ENCRYPTION_OPS_H
#define ENCRYPTION_OPS_H

#include <node_api.h>

/**
 * Derive an encryption key from passphrase and salt
 * 
 * @param env N-API environment
 * @param info Callback info containing:
 *   - arg[0]: passphrase (string)
 *   - arg[1]: salt (Buffer)
 *   - arg[2]: length (number) - key length in bytes
 * @returns Promise<{ encryptionKeyHandle: external }>
 */
napi_value derive_encryption_key(napi_env env, napi_callback_info info);

#endif /* ENCRYPTION_OPS_H */
