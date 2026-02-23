/**
 * @file buffer_helpers.c
 * @brief Buffer utilities implementation
 * 
 * Provides buffer extraction and creation utilities.
 */

#include "buffer_helpers.h"
#include "logger.h"
#include <string.h>

/**
 * Try to extract data from a Node.js Buffer.
 * @return napi_ok on success, napi_invalid_arg if not a Buffer.
 */
static napi_status try_extract_node_buffer(napi_env env, napi_value js_buffer,
                                           void** out_data, size_t* out_length) {
    bool is_buffer;
    if (napi_is_buffer(env, js_buffer, &is_buffer) != napi_ok || !is_buffer) {
        return napi_invalid_arg;
    }
    if (napi_get_buffer_info(env, js_buffer, out_data, out_length) != napi_ok) {
        return napi_invalid_arg;
    }
    LOG_TRACE("Extracted Buffer: %zu bytes", *out_length);
    return napi_ok;
}

/**
 * Try to extract data from an ArrayBuffer.
 * @return napi_ok on success, napi_invalid_arg if not an ArrayBuffer.
 */
static napi_status try_extract_arraybuffer(napi_env env, napi_value js_buffer,
                                           void** out_data, size_t* out_length) {
    bool is_arraybuffer;
    if (napi_is_arraybuffer(env, js_buffer, &is_arraybuffer) != napi_ok || !is_arraybuffer) {
        return napi_invalid_arg;
    }
    if (napi_get_arraybuffer_info(env, js_buffer, out_data, out_length) != napi_ok) {
        return napi_invalid_arg;
    }
    LOG_TRACE("Extracted ArrayBuffer: %zu bytes", *out_length);
    return napi_ok;
}

/**
 * Try to extract data from a TypedArray (e.g. Uint8Array).
 * @return napi_ok on success, napi_invalid_arg if not a TypedArray.
 */
static napi_status try_extract_typedarray(napi_env env, napi_value js_buffer,
                                          void** out_data, size_t* out_length) {
    bool is_typedarray;
    if (napi_is_typedarray(env, js_buffer, &is_typedarray) != napi_ok || !is_typedarray) {
        return napi_invalid_arg;
    }
    napi_typedarray_type type;
    napi_value arraybuffer;
    size_t byte_offset;
    if (napi_get_typedarray_info(env, js_buffer, &type, out_length,
                                 out_data, &arraybuffer, &byte_offset) != napi_ok) {
        return napi_invalid_arg;
    }
    LOG_TRACE("Extracted TypedArray: %zu bytes", *out_length);
    return napi_ok;
}

napi_status extract_buffer(napi_env env, napi_value js_buffer,
                          void** out_data, size_t* out_length) {
    if (try_extract_node_buffer(env, js_buffer, out_data, out_length) == napi_ok) {
        return napi_ok;
    }
    if (try_extract_arraybuffer(env, js_buffer, out_data, out_length) == napi_ok) {
        return napi_ok;
    }
    if (try_extract_typedarray(env, js_buffer, out_data, out_length) == napi_ok) {
        return napi_ok;
    }

    LOG_ERROR("Value is not a Buffer, ArrayBuffer, or TypedArray");
    *out_data = NULL;
    *out_length = 0;
    return napi_invalid_arg;
}

napi_value create_buffer_copy(napi_env env, const void* data, size_t length) {
    napi_value result;
    void* buffer_data;
    
    napi_status status = napi_create_buffer(env, length, &buffer_data, &result);
    if (status != napi_ok) {
        LOG_ERROR("Failed to create buffer copy");
        return NULL;
    }
    
    if (data != NULL && length > 0) {
        safe_memcpy(buffer_data, length, data, length);
    }
    
    LOG_TRACE("Created buffer copy: %zu bytes", length);
    return result;
}

napi_value create_buffer_external(napi_env env, void* data, size_t length,
                                  napi_finalize destructor, void* hint) {
    napi_value result;
    
    napi_status status = napi_create_external_buffer(env, length, data,
                                                     destructor, hint, &result);
    if (status != napi_ok) {
        LOG_ERROR("Failed to create external buffer");
        return NULL;
    }
    
    LOG_TRACE("Created external buffer: %zu bytes", length);
    return result;
}

int is_buffer_like(napi_env env, napi_value value) {
    bool result;
    
    if (napi_is_buffer(env, value, &result) == napi_ok && result) return 1;
    if (napi_is_arraybuffer(env, value, &result) == napi_ok && result) return 1;
    if (napi_is_typedarray(env, value, &result) == napi_ok && result) return 1;
    
    return 0;
}

size_t safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_len) {
    if (dest == NULL || src == NULL || dest_size == 0) return 0;

    const size_t n = src_len <= dest_size ? src_len : dest_size;
    memcpy(dest, src, n);
    return n;
}
