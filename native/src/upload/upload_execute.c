/**
 * @file upload_execute.c
 * @brief Execute function implementations for upload async operations
 * 
 * These functions run on worker threads and call uplink-c functions.
 */

#include "upload_execute.h"
#include "upload_types.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== upload_object execute ========== */

void upload_object_execute(napi_env env, void* data) {
    (void)env;
    UploadObjectData* work_data = (UploadObjectData*)data;
    LOG_DEBUG("Starting upload: %s/%s", work_data->bucket_name, work_data->object_key);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    UplinkUploadOptions options = {0};
    UplinkUploadOptions* options_ptr = NULL;
    
    if (work_data->expires > 0) {
        options.expires = work_data->expires;
        options_ptr = &options;
    }
    
    work_data->result = uplink_upload_object(&project, work_data->bucket_name, work_data->object_key, options_ptr);
}

/* ========== upload_write execute ========== */

void upload_write_execute(napi_env env, void* data) {
    (void)env;
    UploadWriteData* work_data = (UploadWriteData*)data;
    LOG_DEBUG("Writing %zu bytes to upload", work_data->data_length);
    
    UplinkUpload upload = { ._handle = work_data->upload_handle };
    uint8_t* buf = (uint8_t*)work_data->buffer_ptr;
    work_data->result = uplink_upload_write(&upload, buf, work_data->data_length);
}

/* ========== upload_commit execute ========== */

void upload_commit_execute(napi_env env, void* data) {
    (void)env;
    UploadFinalizeData* work_data = (UploadFinalizeData*)data;
    LOG_DEBUG("Committing upload (handle=%zu)", work_data->upload_handle);
    
    UplinkUpload upload = { ._handle = work_data->upload_handle };
    work_data->error = uplink_upload_commit(&upload);
}

/* ========== upload_abort execute ========== */

void upload_abort_execute(napi_env env, void* data) {
    (void)env;
    UploadFinalizeData* work_data = (UploadFinalizeData*)data;
    LOG_DEBUG("Aborting upload (handle=%zu)", work_data->upload_handle);
    
    UplinkUpload upload = { ._handle = work_data->upload_handle };
    work_data->error = uplink_upload_abort(&upload);
}

/* ========== upload_set_metadata execute ========== */

void upload_set_metadata_execute(napi_env env, void* data) {
    (void)env;
    UploadMetadataData* work_data = (UploadMetadataData*)data;
    LOG_DEBUG("Setting custom metadata on upload");
    
    UplinkUpload upload = { ._handle = work_data->upload_handle };
    work_data->error = uplink_upload_set_custom_metadata(&upload, work_data->metadata);
}

/* ========== upload_info execute ========== */

void upload_info_execute(napi_env env, void* data) {
    (void)env;
    UploadInfoData* work_data = (UploadInfoData*)data;
    LOG_DEBUG("Getting upload info (handle=%zu)", work_data->upload_handle);
    
    UplinkUpload upload = { ._handle = work_data->upload_handle };
    work_data->result = uplink_upload_info(&upload);
}
