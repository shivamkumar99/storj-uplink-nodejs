/**
 * @file download_complete.c
 * @brief Complete function implementations for download async operations
 * 
 * These functions run on the main thread and handle promise resolution/rejection.
 */

#include "download_complete.h"
#include "download_types.h"
#include "../common/handle_helpers.h"
#include "../common/result_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/object_converter.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== download_object complete ========== */

void download_object_complete(napi_env env, napi_status status, void* data) {
    DownloadObjectData* work_data = (DownloadObjectData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "downloadObject");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("downloadObject failed: %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    /* Create result object with download handle */
    napi_value result_obj;
    napi_create_object(env, &result_obj);
    
    napi_value download_handle = create_handle_external(env, work_data->result.download->_handle, HANDLE_TYPE_DOWNLOAD, work_data->result.download, NULL);
    napi_set_named_property(env, result_obj, "downloadHandle", download_handle);
    
    LOG_INFO("Download started: %s/%s", work_data->bucket_name, work_data->object_key);
    napi_resolve_deferred(env, work_data->deferred, result_obj);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== download_read complete ========== */

void download_read_complete(napi_env env, napi_status status, void* data) {
    DownloadReadData* work_data = (DownloadReadData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "downloadRead");
    
    if (work_data->result.error != NULL) {
        /*
         * Reject on ANY error, including EOF (code == -1).
         * This matches the old uplink-nodejs pattern where the JS caller
         * catches the error to detect EOF and break its read loop.
         * We include bytes_read in the error object so the caller can use
         * any data that was read before the error/EOF occurred.
         */
        const char* msg = work_data->result.error->message;
        if (msg == NULL) { msg = "EOF"; }
        LOG_DEBUG("downloadRead: error code=%d msg=%s bytes_read=%zu",
                  work_data->result.error->code, msg, work_data->result.bytes_read);
        
        napi_value error = create_typed_error(env, work_data->result.error->code,
                                               work_data->result.error->message ? work_data->result.error->message : "EOF");
        
        /* Attach bytes_read to the error object so JS can recover partial data */
        napi_value bytes_read_val;
        napi_create_int64(env, (int64_t)work_data->result.bytes_read, &bytes_read_val);
        napi_set_named_property(env, error, "bytesRead", bytes_read_val);
        
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    /* Success: resolve with { bytesRead: N } */
    napi_value result_obj;
    napi_create_object(env, &result_obj);
    
    napi_value bytes_read;
    napi_create_int64(env, (int64_t)work_data->result.bytes_read, &bytes_read);
    napi_set_named_property(env, result_obj, "bytesRead", bytes_read);
    
    LOG_DEBUG("downloadRead: success bytes_read=%zu", work_data->result.bytes_read);
    napi_resolve_deferred(env, work_data->deferred, result_obj);
    
cleanup:
    /* Release buffer reference */
    if (work_data->buffer_ref) {
        napi_delete_reference(env, work_data->buffer_ref);
    }
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== download_info complete ========== */

void download_info_complete(napi_env env, napi_status status, void* data) {
    DownloadInfoData* work_data = (DownloadInfoData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "downloadInfo");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("downloadInfo failed: %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value result = uplink_object_to_js(env, work_data->result.object);
    LOG_INFO("download_info complete: key=%s", work_data->result.object ? work_data->result.object->key : "(null)");
    napi_resolve_deferred(env, work_data->deferred, result);
    uplink_free_object_result(work_data->result);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== close_download complete ========== */

void close_download_complete(napi_env env, napi_status status, void* data) {
    CloseDownloadData* work_data = (CloseDownloadData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "closeDownload");
    
    if (work_data->error != NULL) {
        LOG_ERROR("closeDownload failed: %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    LOG_INFO("close_download complete");
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
