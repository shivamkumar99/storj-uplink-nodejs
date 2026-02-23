/**
 * @file result_helpers.h
 * @brief Result and error handling utilities for uplink-nodejs
 * 
 * Provides utilities for converting uplink-c errors to JS errors
 * and creating promises.
 */

#ifndef UPLINK_RESULT_HELPERS_H
#define UPLINK_RESULT_HELPERS_H

#include <node_api.h>
#include <stdint.h>

/* Error codes from uplink-c */
#define UPLINK_ERROR_INTERNAL               0x02
#define UPLINK_ERROR_CANCELED               0x03
#define UPLINK_ERROR_INVALID_HANDLE         0x04
#define UPLINK_ERROR_TOO_MANY_REQUESTS      0x05
#define UPLINK_ERROR_BANDWIDTH_LIMIT_EXCEEDED 0x06
#define UPLINK_ERROR_STORAGE_LIMIT_EXCEEDED   0x07
#define UPLINK_ERROR_SEGMENTS_LIMIT_EXCEEDED  0x08
#define UPLINK_ERROR_PERMISSION_DENIED      0x09
#define UPLINK_ERROR_BUCKET_NAME_INVALID    0x10
#define UPLINK_ERROR_BUCKET_ALREADY_EXISTS  0x11
#define UPLINK_ERROR_BUCKET_NOT_EMPTY       0x12
#define UPLINK_ERROR_BUCKET_NOT_FOUND       0x13
#define UPLINK_ERROR_OBJECT_KEY_INVALID     0x20
#define UPLINK_ERROR_OBJECT_NOT_FOUND       0x21
#define UPLINK_ERROR_UPLOAD_DONE            0x22

/**
 * Simplified UplinkError structure for use in helpers
 */
typedef struct {
    int32_t code;
    char* message;
} UplinkErrorSimple;

/**
 * Convert UplinkError to JS Error object
 * 
 * @param env N-API environment
 * @param error Uplink error (can be NULL)
 * @return JS Error object or NULL
 */
napi_value uplink_error_to_js(napi_env env, UplinkErrorSimple* error);

/**
 * Create a rejected promise from error
 * 
 * @param env N-API environment
 * @param code Error code
 * @param message Error message
 * @return Rejected promise
 */
napi_value create_rejected_promise_with_code(napi_env env, int32_t code, const char* message);

/**
 * Create a rejected promise from error message
 * 
 * @param env N-API environment
 * @param message Error message
 * @return Rejected promise
 */
napi_value create_rejected_promise(napi_env env, const char* message);

/**
 * Create a resolved promise with value
 * 
 * @param env N-API environment
 * @param value Value to resolve with
 * @return Resolved promise
 */
napi_value create_resolved_promise(napi_env env, napi_value value);

/**
 * Get error code name
 * 
 * @param code Error code
 * @return Error name string
 */
const char* get_error_name(int32_t code);

/**
 * Throw JS error and return NULL
 * Useful for synchronous error paths
 * 
 * @param env N-API environment
 * @param message Error message
 * @return NULL (for convenience in return statements)
 */
napi_value throw_error(napi_env env, const char* message);

/**
 * Throw JS type error and return NULL
 * 
 * @param env N-API environment
 * @param message Error message
 * @return NULL
 */
napi_value throw_type_error(napi_env env, const char* message);

#endif /* UPLINK_RESULT_HELPERS_H */
