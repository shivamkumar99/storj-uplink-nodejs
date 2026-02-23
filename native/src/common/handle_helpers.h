/**
 * @file handle_helpers.h
 * @brief Handle management utilities for uplink-nodejs
 * 
 * Provides type-safe handle creation and extraction for native handles.
 */

#ifndef UPLINK_HANDLE_HELPERS_H
#define UPLINK_HANDLE_HELPERS_H

#include <node_api.h>
#include <stddef.h>

/**
 * Handle types for type-safe handle management
 */
typedef enum {
    HANDLE_TYPE_ACCESS,
    HANDLE_TYPE_PROJECT,
    HANDLE_TYPE_DOWNLOAD,
    HANDLE_TYPE_UPLOAD,
    HANDLE_TYPE_ENCRYPTION_KEY,
    HANDLE_TYPE_PART_UPLOAD,
    HANDLE_TYPE_OBJECT_ITERATOR,
    HANDLE_TYPE_BUCKET_ITERATOR,
    HANDLE_TYPE_UPLOAD_ITERATOR,
    HANDLE_TYPE_PART_ITERATOR
} HandleType;

/**
 * Handle wrapper structure for type-safe handle management.
 * 
 * @field type      The type of the handle (for runtime type checking)
 * @field handle    The uplink-c handle value (size_t key into Go universe map)
 * @field native_ptr  Pointer to the uplink-c allocated struct (e.g., UplinkAccess*).
 *                    Stored so the destructor can call uplink_free_*_result to properly
 *                    release both the C struct and the Go-side universe handle.
 *                    NULL for iterator handles (which store the pointer as the handle itself).
 */
typedef struct {
    HandleType type;
    size_t handle;
    void* native_ptr;
} HandleWrapper;

/**
 * Create a JS external from a handle
 * @param env N-API environment
 * @param handle The handle value (size_t)
 * @param type The type of handle
 * @param native_ptr Pointer to the uplink-c allocated struct (for proper cleanup), or NULL
 * @param destructor Optional destructor function
 * @return napi_value representing the external, or NULL on error
 */
napi_value create_handle_external(napi_env env, size_t handle, 
                                  HandleType type, void* native_ptr,
                                  napi_finalize destructor);

/**
 * Extract handle from JS external
 * @param env N-API environment
 * @param js_value The JS external value
 * @param type Expected handle type (for validation)
 * @param out_handle Output for the handle value
 * @return napi_ok on success, error status otherwise
 */
napi_status extract_handle(napi_env env, napi_value js_value,
                          HandleType type, size_t* out_handle);

/**
 * Validate that a handle is non-zero
 * @param handle The handle to validate
 * @return 1 if valid, 0 if invalid
 */
int validate_handle(size_t handle);

/**
 * Get string name for handle type
 * @param type The handle type
 * @return Human-readable name
 */
const char* get_handle_type_name(HandleType type);

#endif /* UPLINK_HANDLE_HELPERS_H */
