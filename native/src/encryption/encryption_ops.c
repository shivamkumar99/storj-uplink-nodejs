/**
 * @file encryption_ops.c
 * @brief N-API entry points for encryption operations
 * 
 * This file contains the public N-API functions that are exposed to JavaScript.
 * These functions:
 * - Extract and validate arguments from JavaScript
 * - Allocate and initialize async work data
 * - Create promises and queue async work
 * 
 * The actual work is done in:
 * - encryption_execute.c (worker thread functions)
 * - encryption_complete.c (main thread completion handlers)
 */

#include "encryption_ops.h"
#include "encryption_types.h"
#include "encryption_execute.h"
#include "encryption_complete.h"
#include "../common/string_helpers.h"
#include "../common/buffer_helpers.h"
#include "../common/result_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== deriveEncryptionKey ========== */

napi_value derive_encryption_key(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    LOG_DEBUG("derive_encryption_key called with %zu args", argc);
    
    if (argc < 2) {
        return throw_type_error(env, "passphrase and salt are required");
    }
    
    /* Extract passphrase */
    char* passphrase = NULL;
    if (extract_string_required(env, argv[0], "passphrase", &passphrase) != napi_ok) {
        return NULL;
    }
    
    /* Extract salt buffer */
    void* salt_data = NULL;
    size_t salt_length = 0;
    if (extract_buffer(env, argv[1], &salt_data, &salt_length) != napi_ok) {
        free(passphrase);
        return throw_type_error(env, "salt must be a Buffer");
    }
    
    /* Copy salt data (to keep it alive during async execution) */
    void* salt_copy = malloc(salt_length);
    if (salt_copy == NULL) {
        free(passphrase);
        return throw_error(env, "Out of memory");
    }
    safe_memcpy(salt_copy, salt_length, salt_data, salt_length);
    
    /* Allocate work data */
    DeriveKeyData* work_data = (DeriveKeyData*)calloc(1, sizeof(DeriveKeyData));
    if (!work_data) {
        free(passphrase);
        free(salt_copy);
        return throw_error(env, "Out of memory");
    }
    
    work_data->passphrase = passphrase;
    work_data->salt = salt_copy;
    work_data->salt_length = salt_length;
    
    /* Create promise */
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    /* Create async work */
    napi_value work_name;
    napi_create_string_utf8(env, "deriveEncryptionKey", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, derive_key_execute, derive_key_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}
