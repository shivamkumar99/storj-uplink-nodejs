/**
 * @file handle_helpers.c
 * @brief Handle management utilities implementation
 * 
 * Provides type-safe handle creation and extraction for native handles.
 * Includes destructors that properly free uplink-c resources when
 * JavaScript externals are garbage collected.
 */

#include "handle_helpers.h"
#include "logger.h"

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

#include <stdlib.h>

static const char* handle_type_names[] = {
    "Access",
    "Project", 
    "Download",
    "Upload",
    "EncryptionKey",
    "PartUpload",
    "ObjectIterator",
    "BucketIterator",
    "UploadIterator",
    "PartIterator"
};

const char* get_handle_type_name(HandleType type) {
    if (type >= 0 && type < (int)(sizeof(handle_type_names)/sizeof(handle_type_names[0]))) {
        return handle_type_names[type];
    }
    return "Unknown";
}

/**
 * Free uplink-c resources associated with the handle's native pointer.
 * 
 * Each handle type has a corresponding uplink_free_*_result function that
 * releases both the C-allocated struct and the Go-side universe handle.
 * For types with explicit close operations (project, download, upload),
 * the free function also closes if not already closed.
 */
static void free_native_resource(HandleType type, void* native_ptr) {
    switch (type) {
        case HANDLE_TYPE_ACCESS: {
            UplinkAccessResult result = {
                .access = (UplinkAccess*)native_ptr, .error = NULL
            };
            uplink_free_access_result(result);
            break;
        }
        case HANDLE_TYPE_PROJECT: {
            UplinkProjectResult result = {
                .project = (UplinkProject*)native_ptr, .error = NULL
            };
            uplink_free_project_result(result);
            break;
        }
        case HANDLE_TYPE_DOWNLOAD: {
            UplinkDownloadResult result = {
                .download = (UplinkDownload*)native_ptr, .error = NULL
            };
            uplink_free_download_result(result);
            break;
        }
        case HANDLE_TYPE_UPLOAD: {
            UplinkUploadResult result = {
                .upload = (UplinkUpload*)native_ptr, .error = NULL
            };
            uplink_free_upload_result(result);
            break;
        }
        case HANDLE_TYPE_ENCRYPTION_KEY: {
            UplinkEncryptionKeyResult result = {
                .encryption_key = (UplinkEncryptionKey*)native_ptr, .error = NULL
            };
            uplink_free_encryption_key_result(result);
            break;
        }
        case HANDLE_TYPE_PART_UPLOAD: {
            UplinkPartUploadResult result = {
                .part_upload = (UplinkPartUpload*)native_ptr, .error = NULL
            };
            uplink_free_part_upload_result(result);
            break;
        }
        default:
            /* Iterator handles store the pointer as the handle itself and
             * are freed explicitly (e.g., freeBucketIterator). No action needed. */
            LOG_TRACE("No native resource to free for %s handle", get_handle_type_name(type));
            break;
    }
}

/**
 * Default destructor: frees wrapper and, if a native pointer is stored,
 * releases the corresponding uplink-c resource via the appropriate
 * uplink_free_*_result function.
 */
static void handle_destructor(napi_env env, void* data, void* hint) {
    (void)env;
    (void)hint;
    
    HandleWrapper* wrapper = (HandleWrapper*)data;
    if (wrapper != NULL) {
        LOG_TRACE("Destroying %s handle wrapper: %zu", 
                  get_handle_type_name(wrapper->type), wrapper->handle);
        
        if (wrapper->native_ptr != NULL) {
            free_native_resource(wrapper->type, wrapper->native_ptr);
            LOG_DEBUG("Freed uplink-c %s resources for handle: %zu",
                      get_handle_type_name(wrapper->type), wrapper->handle);
        }
        
        free(wrapper);
    }
}

napi_value create_handle_external(napi_env env, size_t handle, 
                                  HandleType type, void* native_ptr,
                                  napi_finalize destructor) {
    /* Allocate wrapper */
    HandleWrapper* wrapper = (HandleWrapper*)malloc(sizeof(HandleWrapper));
    if (wrapper == NULL) {
        LOG_ERROR("Failed to allocate handle wrapper for %s", get_handle_type_name(type));
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    wrapper->type = type;
    wrapper->handle = handle;
    wrapper->native_ptr = native_ptr;
    
    /* Create external */
    napi_value external;
    napi_finalize final_destructor = destructor ? destructor : handle_destructor;
    
    napi_status status = napi_create_external(env, wrapper, final_destructor, NULL, &external);
    if (status != napi_ok) {
        LOG_ERROR("Failed to create external for %s handle", get_handle_type_name(type));
        free(wrapper);
        return NULL;
    }
    
    LOG_DEBUG("Created %s handle external: %zu", get_handle_type_name(type), handle);
    return external;
}

napi_status extract_handle(napi_env env, napi_value js_value,
                          HandleType type, size_t* out_handle) {
    void* data = NULL;
    
    napi_status status = napi_get_value_external(env, js_value, &data);
    if (status != napi_ok || data == NULL) {
        LOG_ERROR("Failed to extract %s handle - invalid external", get_handle_type_name(type));
        return napi_invalid_arg;
    }

    const HandleWrapper* wrapper = (const HandleWrapper*)data;

    /* Type check */
    if (wrapper->type != type) {
        LOG_ERROR("Handle type mismatch: expected %s, got %s",
                  get_handle_type_name(type), get_handle_type_name(wrapper->type));
        return napi_invalid_arg;
    }
    
    /* Validate handle */
    if (wrapper->handle == 0) {
        LOG_ERROR("Invalid %s handle (zero)", get_handle_type_name(type));
        return napi_invalid_arg;
    }
    
    *out_handle = wrapper->handle;
    LOG_TRACE("Extracted %s handle: %zu", get_handle_type_name(type), *out_handle);
    
    return napi_ok;
}

int validate_handle(size_t handle) {
    return handle != 0;
}
