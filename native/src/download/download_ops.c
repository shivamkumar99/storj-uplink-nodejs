/**
 * @file download_ops.c
 * @brief N-API entry points for download operations
 * 
 * This file contains the N-API entry point functions that:
 * 1. Extract arguments from JavaScript
 * 2. Validate inputs
 * 3. Set up async work
 * 4. Return promises
 */

#include "download_ops.h"
#include "download_types.h"
#include "download_execute.h"
#include "download_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/buffer_helpers.h"
#include "../common/result_helpers.h"
#include "../common/type_converters.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== download_object ========== */

napi_value download_object(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    LOG_DEBUG("download_object called with %zu args", argc);
    
    if (argc < 3) {
        return throw_type_error(env, "project, bucket, and key are required");
    }
    
    /* Extract project handle */
    size_t project_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle) != napi_ok) {
        return throw_type_error(env, "Invalid project handle");
    }
    
    /* Extract bucket name and object key */
    char *bucket_name = NULL, *object_key = NULL;
    if (extract_string_required(env, argv[1], "bucket", &bucket_name) != napi_ok ||
        extract_string_required(env, argv[2], "key", &object_key) != napi_ok) {
        free(bucket_name);
        free(object_key);
        return NULL;
    }
    
    /* Extract options (optional) */
    int64_t offset = 0;
    int64_t length = -1; /* -1 means read to end */
    
    if (argc > 3) {
        napi_valuetype type;
        napi_typeof(env, argv[3], &type);
        if (type == napi_object) {
            offset = get_int64_property(env, argv[3], "offset", 0);
            length = get_int64_property(env, argv[3], "length", -1);
        }
    }
    
    /* Allocate work data */
    DownloadObjectData* work_data = (DownloadObjectData*)calloc(1, sizeof(DownloadObjectData));
    if (!work_data) {
        free(bucket_name);
        free(object_key);
        return throw_error(env, "Out of memory");
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->offset = offset;
    work_data->length = length;
    
    /* Create promise */
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    /* Create async work */
    napi_value work_name;
    napi_create_string_utf8(env, "downloadObject", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, download_object_execute, download_object_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== download_read ========== */

napi_value download_read(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    LOG_DEBUG("download_read called with %zu args", argc);
    
    if (argc < 3) {
        return throw_type_error(env, "download handle, buffer, and length are required");
    }
    
    /* Extract download handle */
    size_t download_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_DOWNLOAD, &download_handle) != napi_ok) {
        return throw_type_error(env, "Invalid download handle");
    }
    
    /* Extract buffer */
    bool is_buffer;
    napi_is_buffer(env, argv[1], &is_buffer);
    if (!is_buffer) {
        return throw_type_error(env, "Second argument must be a Buffer");
    }
    
    void* buffer;
    size_t buffer_length;
    napi_get_buffer_info(env, argv[1], &buffer, &buffer_length);
    
    /* Extract length to read */
    napi_valuetype length_type;
    napi_typeof(env, argv[2], &length_type);
    if (length_type != napi_number) {
        return throw_type_error(env, "length must be a number");
    }
    
    int64_t length;
    napi_get_value_int64(env, argv[2], &length);
    
    if (length < 0 || (size_t)length > buffer_length) {
        return throw_error(env, "Length exceeds buffer size");
    }
    
    /* Allocate work data */
    DownloadReadData* work_data = (DownloadReadData*)calloc(1, sizeof(DownloadReadData));
    if (!work_data) {
        return throw_error(env, "Out of memory");
    }
    
    work_data->download_handle = download_handle;
    work_data->buffer_ptr = buffer;  /* Point to the JS buffer directly */
    work_data->data_length = (size_t)length;
    
    /* Create reference to keep buffer alive during async work */
    napi_create_reference(env, argv[1], 1, &work_data->buffer_ref);
    
    /* Create promise */
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    /* Create async work */
    napi_value work_name;
    napi_create_string_utf8(env, "downloadRead", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, download_read_execute, download_read_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== download_info ========== */

napi_value download_info(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    LOG_DEBUG("download_info called with %zu args", argc);
    
    if (argc < 1) {
        return throw_type_error(env, "download handle is required");
    }
    
    /* Extract download handle */
    size_t download_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_DOWNLOAD, &download_handle) != napi_ok) {
        return throw_type_error(env, "Invalid download handle");
    }
    
    /* Allocate work data */
    DownloadInfoData* work_data = (DownloadInfoData*)calloc(1, sizeof(DownloadInfoData));
    if (!work_data) {
        return throw_error(env, "Out of memory");
    }
    
    work_data->download_handle = download_handle;
    
    /* Create promise */
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    /* Create async work */
    napi_value work_name;
    napi_create_string_utf8(env, "downloadInfo", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, download_info_execute, download_info_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== close_download ========== */

napi_value close_download(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    LOG_DEBUG("close_download called with %zu args", argc);
    
    if (argc < 1) {
        return throw_type_error(env, "download handle is required");
    }
    
    /* Extract download handle */
    size_t download_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_DOWNLOAD, &download_handle) != napi_ok) {
        return throw_type_error(env, "Invalid download handle");
    }
    
    /* Allocate work data */
    CloseDownloadData* work_data = (CloseDownloadData*)calloc(1, sizeof(CloseDownloadData));
    if (!work_data) {
        return throw_error(env, "Out of memory");
    }
    
    work_data->download_handle = download_handle;
    
    /* Create promise */
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    /* Create async work */
    napi_value work_name;
    napi_create_string_utf8(env, "closeDownload", NAPI_AUTO_LENGTH, &work_name);
    napi_create_async_work(env, NULL, work_name, close_download_execute, close_download_complete, work_data, &work_data->work);
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}
