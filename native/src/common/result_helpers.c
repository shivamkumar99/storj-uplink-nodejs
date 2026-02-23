/**
 * @file result_helpers.c
 * @brief Result and error handling utilities implementation
 * 
 * Provides utilities for converting uplink-c errors to JS errors
 * and creating promises.
 */

#include "result_helpers.h"
#include "string_helpers.h"
#include "error_registry.h"
#include "logger.h"
#include <string.h>

/* Lookup table for error code → name mapping */
typedef struct {
    int32_t code;
    const char* name;
} ErrorNameEntry;

static const ErrorNameEntry ERROR_NAMES[] = {
    { UPLINK_ERROR_INTERNAL,              "InternalError" },
    { UPLINK_ERROR_CANCELED,              "CanceledError" },
    { UPLINK_ERROR_INVALID_HANDLE,        "InvalidHandleError" },
    { UPLINK_ERROR_TOO_MANY_REQUESTS,     "TooManyRequestsError" },
    { UPLINK_ERROR_BANDWIDTH_LIMIT_EXCEEDED, "BandwidthLimitError" },
    { UPLINK_ERROR_STORAGE_LIMIT_EXCEEDED,   "StorageLimitError" },
    { UPLINK_ERROR_SEGMENTS_LIMIT_EXCEEDED,  "SegmentsLimitError" },
    { UPLINK_ERROR_PERMISSION_DENIED,     "PermissionDeniedError" },
    { UPLINK_ERROR_BUCKET_NAME_INVALID,   "BucketNameInvalidError" },
    { UPLINK_ERROR_BUCKET_ALREADY_EXISTS, "BucketAlreadyExistsError" },
    { UPLINK_ERROR_BUCKET_NOT_EMPTY,      "BucketNotEmptyError" },
    { UPLINK_ERROR_BUCKET_NOT_FOUND,      "BucketNotFoundError" },
    { UPLINK_ERROR_OBJECT_KEY_INVALID,    "ObjectKeyInvalidError" },
    { UPLINK_ERROR_OBJECT_NOT_FOUND,      "ObjectNotFoundError" },
    { UPLINK_ERROR_UPLOAD_DONE,           "UploadDoneError" },
};

static const size_t ERROR_NAMES_COUNT = sizeof(ERROR_NAMES) / sizeof(ERROR_NAMES[0]);

const char* get_error_name(int32_t code) {
    for (size_t i = 0; i < ERROR_NAMES_COUNT; i++) {
        if (ERROR_NAMES[i].code == code) {
            return ERROR_NAMES[i].name;
        }
    }
    return "UplinkError";
}

napi_value uplink_error_to_js(napi_env env, UplinkErrorSimple* error) {
    if (error == NULL) {
        return NULL;
    }
    
    napi_value js_error, js_message, js_code, js_name;
    
    /* Create error message */
    const char* message = error->message ? error->message : "Unknown error";
    napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &js_message);
    
    /* Create Error object */
    napi_create_error(env, NULL, js_message, &js_error);
    
    /* Add code property */
    napi_create_int32(env, error->code, &js_code);
    napi_set_named_property(env, js_error, "code", js_code);
    
    /* Add name property */
    const char* error_name = get_error_name(error->code);
    napi_create_string_utf8(env, error_name, NAPI_AUTO_LENGTH, &js_name);
    napi_set_named_property(env, js_error, "name", js_name);
    
    LOG_DEBUG("Created JS error: code=%d (%s), message=%s", 
              error->code, error_name, message);
    
    return js_error;
}

napi_value create_rejected_promise_with_code(napi_env env, int32_t code, const char* message) {
    napi_value promise;
    napi_deferred deferred;
    
    napi_create_promise(env, &deferred, &promise);
    
    napi_value js_error = create_typed_error(env, code, message);
    napi_reject_deferred(env, deferred, js_error);
    
    LOG_DEBUG("Created rejected promise with code: %d", code);
    return promise;
}

napi_value create_rejected_promise(napi_env env, const char* message) {
    napi_value promise;
    napi_deferred deferred;
    
    napi_create_promise(env, &deferred, &promise);
    
    napi_value js_message;
    napi_create_string_utf8(env, message ? message : "Unknown error", NAPI_AUTO_LENGTH, &js_message);
    
    napi_value js_error;
    napi_create_error(env, NULL, js_message, &js_error);
    
    napi_reject_deferred(env, deferred, js_error);
    
    LOG_DEBUG("Created rejected promise");
    return promise;
}

napi_value create_resolved_promise(napi_env env, napi_value value) {
    napi_value promise;
    napi_deferred deferred;
    
    napi_create_promise(env, &deferred, &promise);
    napi_resolve_deferred(env, deferred, value);
    
    LOG_TRACE("Created resolved promise");
    return promise;
}

napi_value throw_error(napi_env env, const char* message) {
    LOG_ERROR("Throwing error: %s", message);
    napi_throw_error(env, NULL, message);
    return NULL;
}

napi_value throw_type_error(napi_env env, const char* message) {
    LOG_ERROR("Throwing type error: %s", message);
    napi_throw_type_error(env, NULL, message);
    return NULL;
}
