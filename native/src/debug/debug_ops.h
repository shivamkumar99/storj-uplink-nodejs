/**
 * @file debug_ops.h
 * @brief Debug operations for uplink-nodejs
 *
 * Provides N-API bindings for uplink-c debug/utility operations.
 */

#ifndef UPLINK_DEBUG_OPS_H
#define UPLINK_DEBUG_OPS_H

#include <node_api.h>

/**
 * Check if internal handle universe is empty
 * JS: internalUniverseIsEmpty() -> Promise<boolean>
 *
 * This is a debug function that returns true if nothing is stored
 * in the global handle map. Useful for testing memory leaks.
 */
napi_value internal_universe_is_empty(napi_env env, napi_callback_info info);

/**
 * Test function: throw a typed error with a given error code.
 * JS: testThrowTypedError(code, message) -> Promise<never>
 *
 * Always rejects with a typed StorjError subclass instance.
 * Used to verify that `instanceof` checks work on native errors.
 *
 * @param code   - int32 error code (e.g. 0x13 for BUCKET_NOT_FOUND)
 * @param message - string error message/details
 */
napi_value test_throw_typed_error(napi_env env, napi_callback_info info);

#endif /* UPLINK_DEBUG_OPS_H */
