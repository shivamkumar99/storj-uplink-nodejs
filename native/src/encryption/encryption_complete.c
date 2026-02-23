/**
 * @file encryption_complete.c
 * @brief Complete functions for encryption async operations
 * 
 * These functions run on the main thread after execute completes.
 * They handle:
 * - Error checking and rejection
 * - Result conversion to JavaScript values
 * - Memory cleanup
 * - Promise resolution/rejection
 */

#include "encryption_complete.h"
#include "encryption_types.h"
#include "../common/handle_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== Complete Functions (Main Thread) ========== */

void derive_key_complete(napi_env env, napi_status status, void* data) {
    DeriveKeyData* work_data = (DeriveKeyData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "deriveEncryptionKey");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("deriveEncryptionKey failed: %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    /* Create encryption key handle external */
    napi_value key_handle = create_handle_external(
        env, 
        work_data->result.encryption_key->_handle, 
        HANDLE_TYPE_ENCRYPTION_KEY, 
        work_data->result.encryption_key,
        NULL
    );
    
    LOG_INFO("Encryption key derived successfully");
    napi_resolve_deferred(env, work_data->deferred, key_handle);
    
cleanup:
    free(work_data->passphrase);
    free(work_data->salt);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
