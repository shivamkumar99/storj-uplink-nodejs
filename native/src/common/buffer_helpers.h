/**
 * @file buffer_helpers.h
 * @brief Buffer utilities for uplink-nodejs native module
 * 
 * Provides buffer extraction and creation utilities.
 */

#ifndef UPLINK_BUFFER_HELPERS_H
#define UPLINK_BUFFER_HELPERS_H

#include <node_api.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Extract buffer data and length from JS Buffer/ArrayBuffer/TypedArray
 * 
 * @param env N-API environment
 * @param js_buffer JS buffer value
 * @param out_data Output for data pointer (NOT copied)
 * @param out_length Output for data length
 * @return napi_ok on success
 */
napi_status extract_buffer(napi_env env, napi_value js_buffer,
                          void** out_data, size_t* out_length);

/**
 * Create JS Buffer by copying data
 * Safe - data can be freed after this call
 * 
 * @param env N-API environment
 * @param data Source data
 * @param length Data length
 * @return JS Buffer or NULL
 */
napi_value create_buffer_copy(napi_env env, const void* data, size_t length);

/**
 * Create JS Buffer using external data (zero-copy)
 * WARNING: data must remain valid until buffer is GC'd
 * 
 * @param env N-API environment
 * @param data Data pointer
 * @param length Data length
 * @param destructor Function to call when buffer is GC'd
 * @param hint Hint passed to destructor
 * @return JS Buffer or NULL
 */
napi_value create_buffer_external(napi_env env, void* data, size_t length,
                                  napi_finalize destructor, void* hint);

/**
 * Check if value is a buffer-like object
 * 
 * @param env N-API environment
 * @param value Value to check
 * @return 1 if buffer-like, 0 otherwise
 */
int is_buffer_like(napi_env env, napi_value value);

/**
 * Safe memcpy that explicitly validates destination capacity (CWE-120).
 *
 * Copies at most @p dest_size bytes from @p src to @p dest.
 * If @p src_len exceeds @p dest_size, only @p dest_size bytes are copied.
 *
 * @param dest       Destination buffer (must not be NULL)
 * @param dest_size  Allocated size of destination buffer
 * @param src        Source data (must not be NULL)
 * @param src_len    Number of bytes to copy
 * @return Number of bytes actually copied
 */
size_t safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_len);

#endif /* UPLINK_BUFFER_HELPERS_H */
