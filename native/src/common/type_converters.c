/**
 * @file type_converters.c
 * @brief Property helper utilities implementation
 * 
 * Provides helpers for extracting properties from JS objects.
 */

#include "type_converters.h"
#include "string_helpers.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

/* ==================== Property Helpers ==================== */

char* get_string_property(napi_env env, napi_value obj, const char* name) {
    napi_value js_val;
    napi_get_named_property(env, obj, name, &js_val);
    
    char* result = NULL;
    extract_string_optional(env, js_val, &result);
    return result;
}

int64_t get_int64_property(napi_env env, napi_value obj, const char* name, int64_t default_val) {
    napi_value js_val;
    napi_status status = napi_get_named_property(env, obj, name, &js_val);
    if (status != napi_ok) return default_val;
    
    napi_valuetype type;
    napi_typeof(env, js_val, &type);
    if (type == napi_undefined || type == napi_null) return default_val;
    if (type != napi_number) return default_val;
    
    int64_t result;
    status = napi_get_value_int64(env, js_val, &result);
    return status == napi_ok ? result : default_val;
}

int get_bool_property(napi_env env, napi_value obj, const char* name, int default_val) {
    napi_value js_val;
    napi_status status = napi_get_named_property(env, obj, name, &js_val);
    if (status != napi_ok) return default_val;
    
    napi_valuetype type;
    napi_typeof(env, js_val, &type);
    if (type == napi_undefined || type == napi_null) return default_val;
    if (type != napi_boolean) return default_val;
    
    bool result;
    status = napi_get_value_bool(env, js_val, &result);
    return status == napi_ok ? (result ? 1 : 0) : default_val;
}

int64_t get_date_property(napi_env env, napi_value obj, const char* name, int64_t default_val) {
    napi_value js_val;
    napi_status status = napi_get_named_property(env, obj, name, &js_val);
    if (status != napi_ok) return default_val;
    
    napi_valuetype type;
    napi_typeof(env, js_val, &type);
    if (type == napi_undefined || type == napi_null) return default_val;
    
    bool is_date;
    napi_is_date(env, js_val, &is_date);
    if (!is_date) return default_val;
    
    double date_value;
    status = napi_get_date_value(env, js_val, &date_value);
    if (status != napi_ok) return default_val;
    
    /* Convert milliseconds to seconds */
    return (int64_t)(date_value / 1000);
}
