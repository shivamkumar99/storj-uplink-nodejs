/**
 * @file cancel_helpers.h
 * @brief Cancellation handling macro for async complete functions
 * 
 * Provides a standardized macro to handle napi_cancelled status in all
 * async complete functions. This eliminates ~60 copies of identical
 * cancellation boilerplate across all modules.
 * 
 * Usage:
 *   void my_complete(napi_env env, napi_status status, void* data) {
 *       MyData* work_data = (MyData*)data;
 *       REJECT_IF_CANCELLED(env, status, work_data->deferred, "myFunction");
 *       // ... rest of complete logic ...
 *   cleanup:
 *       // ... cleanup ...
 *   }
 */

#ifndef UPLINK_CANCEL_HELPERS_H
#define UPLINK_CANCEL_HELPERS_H

#include <node_api.h>
#include "logger.h"

/**
 * REJECT_IF_CANCELLED - Standard cancellation check for async complete functions
 * 
 * Must be used at the top of a complete function, before any error/success handling.
 * Requires a `cleanup:` label in the function for the goto target.
 * 
 * @param env       napi_env from the complete callback
 * @param status    napi_status from the complete callback
 * @param deferred  napi_deferred to reject on cancellation
 * @param func_name String literal for logging (e.g., "parseAccess")
 */
#define REJECT_IF_CANCELLED(env, status, deferred, func_name)           \
    do {                                                                 \
        if ((status) == napi_cancelled) {                               \
            LOG_WARN("%s: operation cancelled", (func_name));           \
            napi_value _cancel_err, _cancel_msg;                        \
            napi_create_string_utf8((env), "Operation cancelled",       \
                                    NAPI_AUTO_LENGTH, &_cancel_msg);    \
            napi_create_error((env), NULL, _cancel_msg, &_cancel_err);  \
            napi_reject_deferred((env), (deferred), _cancel_err);       \
            goto cleanup;                                                \
        }                                                                \
    } while (0)

#endif /* UPLINK_CANCEL_HELPERS_H */
