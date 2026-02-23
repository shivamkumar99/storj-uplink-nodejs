/**
 * @file encryption_execute.c
 * @brief Execute functions for encryption async operations
 * 
 * These functions run on the worker thread and call uplink-c library functions.
 * They should NOT call any N-API functions (except env is passed but unused).
 */

#include "encryption_execute.h"
#include "encryption_types.h"
#include "../common/logger.h"

#include <string.h>

/* ========== Execute Functions (Worker Thread) ========== */

void derive_key_execute(napi_env env, void* data) {
    (void)env;
    DeriveKeyData* work_data = (DeriveKeyData*)data;
    
    LOG_DEBUG("derive_key_execute: passphrase_len=%zu, salt_len=%zu",
              strlen(work_data->passphrase), work_data->salt_length);
    
    /* Call uplink-c */
    work_data->result = uplink_derive_encryption_key(
        work_data->passphrase,
        work_data->salt,
        work_data->salt_length
    );
    
    if (work_data->result.error) {
        LOG_ERROR("derive_key_execute failed: %s", work_data->result.error->message);
    } else {
        LOG_DEBUG("derive_key_execute success: handle=%zu", work_data->result.encryption_key->_handle);
    }
}
