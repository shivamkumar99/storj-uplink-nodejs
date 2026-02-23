/**
 * @file upload_complete.c
 * @brief Complete function implementations for upload async operations
 * 
 * These functions run on the main thread and handle promise resolution/rejection.
 */

#include "upload_complete.h"
#include "upload_types.h"
#include "../common/handle_helpers.h"
#include "../common/result_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/type_converters.h"
#include "../common/object_converter.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== upload_object complete ========== */

void upload_object_complete(napi_env env, napi_status status, void* data) {
    UploadObjectData* work_data = (UploadObjectData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadObject");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("uploadObject failed: %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value upload_handle = create_handle_external(env, work_data->result.upload->_handle, HANDLE_TYPE_UPLOAD, work_data->result.upload, NULL);
    LOG_INFO("Upload started: %s/%s", work_data->bucket_name, work_data->object_key);
    napi_resolve_deferred(env, work_data->deferred, upload_handle);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== upload_write complete ========== */

void upload_write_complete(napi_env env, napi_status status, void* data) {
    UploadWriteData* work_data = (UploadWriteData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadWrite");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("uploadWrite failed: %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value bytes_written;
    napi_create_int64(env, (int64_t)work_data->result.bytes_written, &bytes_written);
    napi_resolve_deferred(env, work_data->deferred, bytes_written);
    
cleanup:
    /* Release buffer reference (no malloc'd copy to free) */
    if (work_data->buffer_ref) {
        napi_delete_reference(env, work_data->buffer_ref);
    }
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== upload_commit complete ========== */

void upload_commit_complete(napi_env env, napi_status status, void* data) {
    UploadFinalizeData* work_data = (UploadFinalizeData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadCommit");
    
    if (work_data->error != NULL) {
        LOG_ERROR("uploadCommit failed: %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== upload_abort complete ========== */

void upload_abort_complete(napi_env env, napi_status status, void* data) {
    UploadFinalizeData* work_data = (UploadFinalizeData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadAbort");
    
    if (work_data->error != NULL) {
        LOG_ERROR("uploadAbort failed: %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== upload_set_metadata complete ========== */

void upload_set_metadata_complete(napi_env env, napi_status status, void* data) {
    UploadMetadataData* work_data = (UploadMetadataData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadSetCustomMetadata");
    
    if (work_data->error != NULL) {
        LOG_ERROR("uploadSetCustomMetadata failed: %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    for (size_t i = 0; i < work_data->metadata.count; i++) {
        free((void*)work_data->metadata.entries[i].key);
        free((void*)work_data->metadata.entries[i].value);
    }
    free(work_data->metadata.entries);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== upload_info complete ========== */

void upload_info_complete(napi_env env, napi_status status, void* data) {
    UploadInfoData* work_data = (UploadInfoData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadInfo");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("uploadInfo failed: %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value object_obj = uplink_object_to_js(env, work_data->result.object);
    uplink_free_object_result(work_data->result);
    napi_resolve_deferred(env, work_data->deferred, object_obj);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
