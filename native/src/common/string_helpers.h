/**
 * @file string_helpers.h
 * @brief String utilities for uplink-nodejs native module
 * 
 * Provides string extraction, validation, and conversion utilities.
 */

#ifndef UPLINK_STRING_HELPERS_H
#define UPLINK_STRING_HELPERS_H

#include <node_api.h>
#include <stddef.h>

/**
 * Extract C string from JS string
 * Caller MUST free the returned string with free()
 * 
 * @param env N-API environment
 * @param js_string JS string value
 * @param out_str Output for C string (malloc'd)
 * @return napi_ok on success
 */
napi_status extract_string(napi_env env, napi_value js_string, char** out_str);

/**
 * Extract C string with validation (not null, not empty)
 * Throws JS error on validation failure
 * 
 * @param env N-API environment
 * @param js_string JS string value
 * @param param_name Name of parameter (for error messages)
 * @param out_str Output for C string (malloc'd)
 * @return napi_ok on success
 */
napi_status extract_string_required(napi_env env, napi_value js_string, 
                                   const char* param_name, char** out_str);

/**
 * Extract optional C string (returns NULL if undefined/null)
 * 
 * @param env N-API environment
 * @param js_string JS string value
 * @param out_str Output for C string (malloc'd) or NULL
 * @return napi_ok on success
 */
napi_status extract_string_optional(napi_env env, napi_value js_string, char** out_str);

/**
 * Create JS string from C string
 * 
 * @param env N-API environment
 * @param str C string (can be NULL)
 * @return JS string or null
 */
napi_value create_string(napi_env env, const char* str);

/**
 * Validate bucket name format
 * @param bucket_name The bucket name to validate
 * @return 1 if valid, 0 if invalid
 */
int validate_bucket_name(const char* bucket_name);

/**
 * Validate object key format
 * @param object_key The object key to validate
 * @return 1 if valid, 0 if invalid
 */
int validate_object_key(const char* object_key);

/**
 * Free string array
 * @param strings Array of strings
 * @param count Number of strings
 */
void free_string_array(char** strings, size_t count);

#endif /* UPLINK_STRING_HELPERS_H */
