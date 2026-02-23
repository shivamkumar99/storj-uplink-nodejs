/**
 * @file error_registry.c
 * @brief Error class registry — defines StorjError classes entirely in native C
 *
 * Option 2 architecture: all error classes are created inside native module
 * init by running a small embedded JavaScript snippet via napi_run_script.
 * No TypeScript constructors are passed in; the native module is the single
 * source of truth for the class hierarchy.
 *
 * How it works:
 *   1. init_error_classes() runs an inline JS string that defines:
 *        class StorjError extends Error { ... }
 *        class InternalError extends StorjError { ... }
 *        ... (17 subclasses)
 *      and returns an object mapping class names to constructors.
 *
 *   2. The constructors are stored as persistent napi_ref values.
 *
 *   3. create_typed_error(env, code, message) looks up the right constructor
 *      by error code and calls `new XxxError(details)` via napi_new_instance.
 *
 *   4. export_error_classes() attaches every constructor onto the native
 *      module's exports object so TypeScript can import them:
 *        const { StorjError, BucketNotFoundError } = native;
 *
 *   5. instanceof works perfectly because the JS engine owns the entire
 *      prototype chain — there is no bridging or wrapping.
 */

#include "error_registry.h"
#include "result_helpers.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>

/* ========== Embedded JS class hierarchy ========== */

/**
 * JavaScript source that defines the full StorjError class hierarchy.
 *
 * It runs as an IIFE and returns an object whose keys are class names
 * and values are the constructor functions.
 *
 * Each subclass constructor accepts a single `details` argument.
 * The base StorjError constructor accepts (message, code, details).
 *
 * Error.captureStackTrace is used when available (V8 engines) to produce
 * clean stack traces that start from the caller, not from the constructor.
 */
/**
 * JavaScript source that defines the full StorjError class hierarchy.
 *
 * It's structured as a function that takes `ErrorBase` (the global Error
 * constructor) as a parameter, so the classes are guaranteed to extend
 * the correct realm's Error — even when loaded inside Jest or other
 * environments that use separate VM sandboxes.
 *
 * Returns an object whose keys are class names and values are constructors.
 *
 * Error.captureStackTrace is used when available (V8 engines) to produce
 * clean stack traces that start from the caller, not from the constructor.
 */
static const char ERROR_CLASSES_JS[] =
    "(function(ErrorBase) {\n"
    "  'use strict';\n"
    "\n"
    "  class StorjError extends ErrorBase {\n"
    "    constructor(message, code, details) {\n"
    "      super(details != null && details !== '' ? message + ': ' + details : message);\n"
    "      this.name = this.constructor.name;\n"
    "      this.code = code;\n"
    "      this.details = details;\n"
    "      if (typeof ErrorBase.captureStackTrace === 'function') {\n"
    "        ErrorBase.captureStackTrace(this, this.constructor);\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "\n"
    "  /* --- General errors --- */\n"
    "  class InternalError extends StorjError {\n"
    "    constructor(details) { super('Internal error', 0x02, details); }\n"
    "  }\n"
    "  class CanceledError extends StorjError {\n"
    "    constructor(details) { super('Operation canceled', 0x03, details); }\n"
    "  }\n"
    "  class InvalidHandleError extends StorjError {\n"
    "    constructor(details) { super('Invalid handle', 0x04, details); }\n"
    "  }\n"
    "  class TooManyRequestsError extends StorjError {\n"
    "    constructor(details) { super('Too many requests', 0x05, details); }\n"
    "  }\n"
    "  class BandwidthLimitExceededError extends StorjError {\n"
    "    constructor(details) { super('Bandwidth limit exceeded', 0x06, details); }\n"
    "  }\n"
    "  class StorageLimitExceededError extends StorjError {\n"
    "    constructor(details) { super('Storage limit exceeded', 0x07, details); }\n"
    "  }\n"
    "  class SegmentsLimitExceededError extends StorjError {\n"
    "    constructor(details) { super('Segments limit exceeded', 0x08, details); }\n"
    "  }\n"
    "  class PermissionDeniedError extends StorjError {\n"
    "    constructor(details) { super('Permission denied', 0x09, details); }\n"
    "  }\n"
    "\n"
    "  /* --- Bucket errors --- */\n"
    "  class BucketNameInvalidError extends StorjError {\n"
    "    constructor(details) { super('Invalid bucket name', 0x10, details); }\n"
    "  }\n"
    "  class BucketAlreadyExistsError extends StorjError {\n"
    "    constructor(details) { super('Bucket already exists', 0x11, details); }\n"
    "  }\n"
    "  class BucketNotEmptyError extends StorjError {\n"
    "    constructor(details) { super('Bucket is not empty', 0x12, details); }\n"
    "  }\n"
    "  class BucketNotFoundError extends StorjError {\n"
    "    constructor(details) { super('Bucket not found', 0x13, details); }\n"
    "  }\n"
    "\n"
    "  /* --- Object errors --- */\n"
    "  class ObjectKeyInvalidError extends StorjError {\n"
    "    constructor(details) { super('Invalid object key', 0x20, details); }\n"
    "  }\n"
    "  class ObjectNotFoundError extends StorjError {\n"
    "    constructor(details) { super('Object not found', 0x21, details); }\n"
    "  }\n"
    "  class UploadDoneError extends StorjError {\n"
    "    constructor(details) { super('Upload already done', 0x22, details); }\n"
    "  }\n"
    "\n"
    "  /* --- Edge errors --- */\n"
    "  class EdgeAuthDialFailedError extends StorjError {\n"
    "    constructor(details) { super('Edge auth dial failed', 0x30, details); }\n"
    "  }\n"
    "  class EdgeRegisterAccessFailedError extends StorjError {\n"
    "    constructor(details) { super('Edge register access failed', 0x31, details); }\n"
    "  }\n"
    "\n"
    "  return {\n"
    "    StorjError: StorjError,\n"
    "    InternalError: InternalError,\n"
    "    CanceledError: CanceledError,\n"
    "    InvalidHandleError: InvalidHandleError,\n"
    "    TooManyRequestsError: TooManyRequestsError,\n"
    "    BandwidthLimitExceededError: BandwidthLimitExceededError,\n"
    "    StorageLimitExceededError: StorageLimitExceededError,\n"
    "    SegmentsLimitExceededError: SegmentsLimitExceededError,\n"
    "    PermissionDeniedError: PermissionDeniedError,\n"
    "    BucketNameInvalidError: BucketNameInvalidError,\n"
    "    BucketAlreadyExistsError: BucketAlreadyExistsError,\n"
    "    BucketNotEmptyError: BucketNotEmptyError,\n"
    "    BucketNotFoundError: BucketNotFoundError,\n"
    "    ObjectKeyInvalidError: ObjectKeyInvalidError,\n"
    "    ObjectNotFoundError: ObjectNotFoundError,\n"
    "    UploadDoneError: UploadDoneError,\n"
    "    EdgeAuthDialFailedError: EdgeAuthDialFailedError,\n"
    "    EdgeRegisterAccessFailedError: EdgeRegisterAccessFailedError\n"
    "  };\n"
    "});\n";

/* ========== Error Registry Storage ========== */

/**
 * Entry mapping an uplink-c error code to its JS constructor reference.
 */
typedef struct {
    int32_t code;
    const char* name;
    napi_ref constructor_ref;
} ErrorClassEntry;

/**
 * Static registry of error classes.
 * Order matches uplink_definitions.h.
 * StorjError (base) is stored at index 0 (code 0 — not a real uplink code).
 */
static ErrorClassEntry error_registry[] = {
    { 0,                                     "StorjError",                    NULL },
    { UPLINK_ERROR_INTERNAL,                 "InternalError",                 NULL },
    { UPLINK_ERROR_CANCELED,                 "CanceledError",                 NULL },
    { UPLINK_ERROR_INVALID_HANDLE,           "InvalidHandleError",            NULL },
    { UPLINK_ERROR_TOO_MANY_REQUESTS,        "TooManyRequestsError",          NULL },
    { UPLINK_ERROR_BANDWIDTH_LIMIT_EXCEEDED, "BandwidthLimitExceededError",   NULL },
    { UPLINK_ERROR_STORAGE_LIMIT_EXCEEDED,   "StorageLimitExceededError",     NULL },
    { UPLINK_ERROR_SEGMENTS_LIMIT_EXCEEDED,  "SegmentsLimitExceededError",    NULL },
    { UPLINK_ERROR_PERMISSION_DENIED,        "PermissionDeniedError",         NULL },
    { UPLINK_ERROR_BUCKET_NAME_INVALID,      "BucketNameInvalidError",        NULL },
    { UPLINK_ERROR_BUCKET_ALREADY_EXISTS,    "BucketAlreadyExistsError",      NULL },
    { UPLINK_ERROR_BUCKET_NOT_EMPTY,         "BucketNotEmptyError",           NULL },
    { UPLINK_ERROR_BUCKET_NOT_FOUND,         "BucketNotFoundError",           NULL },
    { UPLINK_ERROR_OBJECT_KEY_INVALID,       "ObjectKeyInvalidError",         NULL },
    { UPLINK_ERROR_OBJECT_NOT_FOUND,         "ObjectNotFoundError",           NULL },
    { UPLINK_ERROR_UPLOAD_DONE,              "UploadDoneError",               NULL },
    { 0x30,                                  "EdgeAuthDialFailedError",       NULL },
    { 0x31,                                  "EdgeRegisterAccessFailedError", NULL },
};

#define ERROR_REGISTRY_SIZE (sizeof(error_registry) / sizeof(error_registry[0]))

/** Whether init_error_classes has been called successfully */
static int g_registered = 0;

/* ========== Public API ========== */

int error_classes_registered(void) {
    return g_registered;
}

/**
 * Try to register a single error class from the classes object.
 * Returns 1 on success, 0 if the property was missing or not a function.
 */
static int register_one_error_class(napi_env env, napi_value classes_obj,
                                    ErrorClassEntry* entry) {
    napi_value constructor;
    napi_status status = napi_get_named_property(
        env, classes_obj, entry->name, &constructor);
    if (status != napi_ok) {
        LOG_WARN("Failed to get constructor for '%s'", entry->name);
        return 0;
    }

    napi_valuetype ctor_type;
    napi_typeof(env, constructor, &ctor_type);
    if (ctor_type != napi_function) {
        LOG_WARN("'%s' is not a function, skipping", entry->name);
        return 0;
    }

    status = napi_create_reference(env, constructor, 1, &entry->constructor_ref);
    if (status != napi_ok) {
        LOG_ERROR("Failed to create reference for '%s'", entry->name);
        return 0;
    }

    LOG_DEBUG("Registered error class '%s' for code 0x%02x", entry->name, entry->code);
    return 1;
}

/**
 * Get the Error base constructor, either from the argument or globalThis.
 * Returns napi_ok on success, napi_generic_failure if unavailable.
 */
static napi_status get_error_base_constructor(napi_env env, size_t argc,
                                               napi_value* argv,
                                               napi_value* out_ctor) {
    if (argc >= 1) {
        napi_valuetype arg_type;
        napi_typeof(env, argv[0], &arg_type);
        if (arg_type == napi_function) {
            *out_ctor = argv[0];
            LOG_DEBUG("Using caller-provided Error constructor");
            return napi_ok;
        }
        LOG_WARN("initErrorClasses argument is not a function, falling back to globalThis.Error");
    }

    napi_value global;
    napi_status gs = napi_get_global(env, &global);
    if (gs != napi_ok) {
        LOG_ERROR("Failed to get global object");
        return napi_generic_failure;
    }
    gs = napi_get_named_property(env, global, "Error", out_ctor);
    if (gs != napi_ok) {
        LOG_ERROR("Failed to get global Error constructor");
        return napi_generic_failure;
    }
    LOG_DEBUG("Using globalThis.Error constructor");
    return napi_ok;
}

napi_value napi_init_error_classes(napi_env env, napi_callback_info info) {
    LOG_INFO("initErrorClasses called from JS — creating error class hierarchy");

    /* If already initialised, clean up first */
    if (g_registered) {
        error_registry_cleanup(env);
    }

    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    napi_value error_ctor;
    if (get_error_base_constructor(env, argc, argv, &error_ctor) != napi_ok) {
        napi_throw_error(env, NULL, "Failed to get Error constructor");
        return NULL;
    }

    /* Create the JS source string (factory function) */
    napi_value script;
    napi_status status = napi_create_string_utf8(
        env, ERROR_CLASSES_JS, NAPI_AUTO_LENGTH, &script);
    if (status != napi_ok) {
        LOG_ERROR("Failed to create error classes JS string");
        napi_throw_error(env, NULL, "Failed to create error classes JS string");
        return NULL;
    }

    /* Execute the factory function */
    napi_value factory_fn;
    status = napi_run_script(env, script, &factory_fn);
    if (status != napi_ok) {
        LOG_ERROR("Failed to execute error classes JS (napi_run_script)");
        napi_throw_error(env, NULL, "Failed to execute error classes factory script");
        return NULL;
    }

    /* Call factory(Error) to produce the classes object */
    napi_value undefined_this;
    napi_get_undefined(env, &undefined_this);

    napi_value call_args[1] = { error_ctor };
    napi_value classes_obj;
    status = napi_call_function(env, undefined_this, factory_fn, 1, call_args, &classes_obj);
    if (status != napi_ok) {
        LOG_ERROR("Failed to call error classes factory function");
        napi_throw_error(env, NULL, "Failed to call error classes factory function");
        return NULL;
    }

    /* Extract each constructor and store a persistent reference */
    int count = 0;
    for (size_t i = 0; i < ERROR_REGISTRY_SIZE; i++) {
        count += register_one_error_class(env, classes_obj, &error_registry[i]);
    }

    g_registered = 1;
    LOG_INFO("Initialised %d/%d error classes from embedded JS", count, (int)ERROR_REGISTRY_SIZE);

    /* Return the classes object so TS can destructure the constructors */
    return classes_obj;
}

/**
 * Look up a constructor ref by error code.
 * Returns the napi_ref or NULL if not found.
 */
static napi_ref find_error_constructor_ref(int32_t code) {
    for (size_t i = 0; i < ERROR_REGISTRY_SIZE; i++) {
        if (error_registry[i].code == code &&
            error_registry[i].constructor_ref != NULL) {
            return error_registry[i].constructor_ref;
        }
    }
    return NULL;
}

napi_value create_typed_error(napi_env env, int32_t code, const char* message) {
    LOG_DEBUG("create_typed_error: code=0x%02x, message=%s", code,
              message ? message : "(null)");

    if (g_registered) {
        napi_ref ref = find_error_constructor_ref(code);
        if (ref != NULL) {
            napi_value constructor;
            napi_status status = napi_get_reference_value(env, ref, &constructor);

            if (status == napi_ok) {
                napi_value args[1];
                if (message != NULL) {
                    napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &args[0]);
                } else {
                    napi_get_undefined(env, &args[0]);
                }

                napi_value instance;
                status = napi_new_instance(env, constructor, 1, args, &instance);
                if (status == napi_ok) {
                    LOG_DEBUG("Created typed error instance for code 0x%02x", code);
                    return instance;
                }
                LOG_WARN("Failed to instantiate error for code 0x%02x, falling back", code);
            } else {
                LOG_WARN("Failed to get reference for code 0x%02x, falling back", code);
            }
        }
    }

    /*
     * Fallback: classes not initialised or lookup failed.
     * Create a plain Error with code and name properties.
     */
    LOG_DEBUG("Falling back to plain Error for code 0x%02x", code);

    UplinkErrorSimple simple_err = {
        .code = code,
        .message = (char*)(message ? message : "Unknown error")
    };
    return uplink_error_to_js(env, &simple_err);
}

void error_registry_cleanup(napi_env env) {
    LOG_DEBUG("Cleaning up error registry");

    for (size_t i = 0; i < ERROR_REGISTRY_SIZE; i++) {
        if (error_registry[i].constructor_ref != NULL) {
            napi_delete_reference(env, error_registry[i].constructor_ref);
            error_registry[i].constructor_ref = NULL;
        }
    }

    g_registered = 0;
    LOG_INFO("Error registry cleaned up");
}
