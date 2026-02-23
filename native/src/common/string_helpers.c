/**
 * @file string_helpers.c
 * @brief String utilities implementation
 * 
 * Provides string extraction, validation, and conversion utilities.
 */

#include "string_helpers.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

napi_status extract_string(napi_env env, napi_value js_string, char** out_str) {
    size_t str_len;
    napi_status status;
    
    /* Get string length */
    status = napi_get_value_string_utf8(env, js_string, NULL, 0, &str_len);
    if (status != napi_ok) {
        LOG_ERROR("Failed to get string length");
        *out_str = NULL;
        return status;
    }
    
    /* Allocate buffer (+1 for null terminator) */
    *out_str = (char*)malloc(str_len + 1);
    if (*out_str == NULL) {
        LOG_ERROR("Failed to allocate string buffer");
        return napi_generic_failure;
    }
    
    /* Copy string */
    status = napi_get_value_string_utf8(env, js_string, *out_str, str_len + 1, &str_len);
    if (status != napi_ok) {
        LOG_ERROR("Failed to copy string");
        free(*out_str);
        *out_str = NULL;
        return status;
    }
    
    LOG_TRACE("Extracted string: %s", *out_str);
    return napi_ok;
}

napi_status extract_string_required(napi_env env, napi_value js_string, 
                                   const char* param_name, char** out_str) {
    napi_valuetype type;
    napi_typeof(env, js_string, &type);
    
    /* Check for null/undefined */
    if (type == napi_undefined || type == napi_null) {
        LOG_ERROR("Parameter '%s' is required but was null/undefined", param_name);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Parameter '%s' is required", param_name);
        napi_throw_type_error(env, NULL, error_msg);
        *out_str = NULL;
        return napi_invalid_arg;
    }
    
    /* Check type */
    if (type != napi_string) {
        LOG_ERROR("Parameter '%s' must be a string", param_name);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Parameter '%s' must be a string", param_name);
        napi_throw_type_error(env, NULL, error_msg);
        *out_str = NULL;
        return napi_invalid_arg;
    }
    
    /* Extract string */
    napi_status status = extract_string(env, js_string, out_str);
    if (status != napi_ok) {
        return status;
    }
    
    /* Check empty */
    if (*out_str == NULL || strlen(*out_str) == 0) {
        LOG_ERROR("Parameter '%s' cannot be empty", param_name);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Parameter '%s' cannot be empty", param_name);
        napi_throw_type_error(env, NULL, error_msg);
        free(*out_str);
        *out_str = NULL;
        return napi_invalid_arg;
    }
    
    LOG_DEBUG("Validated required string '%s': %s", param_name, *out_str);
    return napi_ok;
}

napi_status extract_string_optional(napi_env env, napi_value js_string, char** out_str) {
    napi_valuetype type;
    napi_typeof(env, js_string, &type);
    
    if (type == napi_undefined || type == napi_null) {
        *out_str = NULL;
        return napi_ok;
    }
    
    return extract_string(env, js_string, out_str);
}

napi_value create_string(napi_env env, const char* str) {
    if (str == NULL) {
        napi_value null_val;
        napi_get_null(env, &null_val);
        return null_val;
    }
    
    napi_value js_str;
    napi_status status = napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &js_str);
    if (status != napi_ok) {
        LOG_ERROR("Failed to create JS string");
        return NULL;
    }
    
    return js_str;
}

int validate_bucket_name(const char* bucket_name) {
    if (bucket_name == NULL) return 0;
    
    size_t len = strlen(bucket_name);
    
    /* Length: 3-63 characters, must start and end with alphanumeric */
    if (len < 3 || len > 63) return 0;
    if (!isalnum((unsigned char)bucket_name[0]) ||
        !isalnum((unsigned char)bucket_name[len - 1])) {
        return 0;
    }
    
    /* Only lowercase letters, numbers, and hyphens */
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)bucket_name[i];
        if (!islower(c) && !isdigit(c) && c != '-') {
            return 0;
        }
    }
    
    return 1;
}

int validate_object_key(const char* object_key) {
    if (object_key == NULL || strlen(object_key) == 0) {
        LOG_DEBUG("Object key is empty");
        return 0;
    }
    
    if (strlen(object_key) > 1024) {
        LOG_DEBUG("Object key too long");
        return 0;
    }
    
    return 1;
}

void free_string_array(char** strings, size_t count) {
    if (strings == NULL) return;
    
    for (size_t i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);
}
