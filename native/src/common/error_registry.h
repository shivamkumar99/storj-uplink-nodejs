/**
 * @file error_registry.h
 * @brief Error class registry — defines StorjError classes entirely in native C
 *
 * Architecture (Option 2 — no TS constructors passed in):
 *
 *   1. The native module exposes an `initErrorClasses()` method. When TS
 *      loads the module it calls this once. Internally it runs an embedded
 *      JS snippet via napi_run_script that defines the full class hierarchy:
 *        class StorjError extends Error { ... }
 *        class InternalError extends StorjError { ... }
 *        ...
 *      The `Error` base is obtained from `napi_get_global(env)` at call
 *      time, so the classes extend the **caller's realm** Error — which
 *      makes `instanceof Error` work even inside Jest VM sandboxes.
 *
 *   2. The constructors are stored as persistent napi_ref values.
 *
 *   3. create_typed_error(env, code, message) looks up the right constructor
 *      by error code and calls `new XxxError(message)` via napi_new_instance.
 *
 *   4. The method returns an object of all constructors so TypeScript can
 *      destructure and re-export them:
 *        const errors = native.initErrorClasses();
 *        // errors.StorjError, errors.BucketNotFoundError, ...
 *
 *   5. instanceof works because JS engine owns the entire prototype chain.
 *
 * Benefits:
 *   - Error classes are defined once, in native, no TS duplication needed
 *   - No passing constructors from TS → native
 *   - instanceof works perfectly since JS engine handles class chain
 *   - TypeScript only needs to re-export / type-augment the native classes
 */

#ifndef UPLINK_ERROR_REGISTRY_H
#define UPLINK_ERROR_REGISTRY_H

#include <node_api.h>
#include <stdint.h>

/**
 * N-API callback: initialize error classes from the caller's realm.
 *
 * Exported as native.initErrorClasses(). Takes zero arguments.
 * Runs embedded JS to create the StorjError class hierarchy, stores
 * constructor refs internally, and returns an object of all constructors.
 *
 * Must be called once after module load from the TS side.
 *
 * @param env N-API environment
 * @param info Callback info (0 args)
 * @return JS object { StorjError, InternalError, ... }
 */
napi_value napi_init_error_classes(napi_env env, napi_callback_info info);

/**
 * Create a typed JS error instance from an error code and message.
 *
 * Looks up the constructor for the given error code and calls
 * `new XxxError(details)`. Falls back to a plain Error if
 * the classes are not initialised.
 *
 * @param env N-API environment
 * @param code Error code from uplink-c (e.g. 0x13 = BUCKET_NOT_FOUND)
 * @param message Error message / details string (may be NULL)
 * @return napi_value — a JS object that is instanceof the correct class
 */
napi_value create_typed_error(napi_env env, int32_t code, const char* message);

/**
 * Check whether error classes have been initialised.
 * @return 1 if initialised, 0 otherwise
 */
int error_classes_registered(void);

/**
 * Clean up persistent references. Call at module teardown.
 * @param env N-API environment
 */
void error_registry_cleanup(napi_env env);

#endif /* UPLINK_ERROR_REGISTRY_H */
