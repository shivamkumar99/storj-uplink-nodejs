/**
 * @file type_converters.h
 * @brief Property helper utilities for uplink-nodejs
 * 
 * Provides helpers for extracting properties from JS objects.
 */

#ifndef UPLINK_TYPE_CONVERTERS_H
#define UPLINK_TYPE_CONVERTERS_H

#include <node_api.h>
#include <stdint.h>
#include <stddef.h>

/* ==================== Property Helpers ==================== */

/**
 * Get string property from JS object
 * Returns malloc'd string - caller must free
 */
char* get_string_property(napi_env env, napi_value obj, const char* name);

/**
 * Get int64 property from JS object
 */
int64_t get_int64_property(napi_env env, napi_value obj, const char* name, int64_t default_val);

/**
 * Get boolean property from JS object
 */
int get_bool_property(napi_env env, napi_value obj, const char* name, int default_val);

/**
 * Get Date property as Unix timestamp (milliseconds) from JS object
 */
int64_t get_date_property(napi_env env, napi_value obj, const char* name, int64_t default_val);

#endif /* UPLINK_TYPE_CONVERTERS_H */
