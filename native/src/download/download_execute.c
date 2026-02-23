/**
 * @file download_execute.c
 * @brief Execute function implementations for download async operations
 * 
 * These functions run on worker threads and call uplink-c functions.
 */

#include "download_execute.h"
#include "download_types.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== download_object execute ========== */

void download_object_execute(napi_env env, void* data) {
    (void)env;
    DownloadObjectData* work_data = (DownloadObjectData*)data;
    
    LOG_DEBUG("download_object_execute: bucket=%s, key=%s, offset=%lld, length=%lld",
              work_data->bucket_name, work_data->object_key, 
              (long long)work_data->offset, (long long)work_data->length);
    
    /* Reconstruct project from handle */
    UplinkProject project = { ._handle = work_data->project_handle };
    
    /* Setup download options */
    UplinkDownloadOptions options = {
        .offset = work_data->offset,
        .length = work_data->length
    };
    
    /* Call uplink-c */
    work_data->result = uplink_download_object(&project, work_data->bucket_name, work_data->object_key, &options);
    
    if (work_data->result.error) {
        LOG_ERROR("download_object_execute failed: %s", work_data->result.error->message);
    } else {
        LOG_DEBUG("download_object_execute success: handle=%zu", work_data->result.download->_handle);
    }
}

/* ========== download_read execute ========== */

void download_read_execute(napi_env env, void* data) {
    (void)env;
    DownloadReadData* work_data = (DownloadReadData*)data;
    
    LOG_DEBUG("download_read_execute: handle=%zu, length=%zu", work_data->download_handle, work_data->data_length);
    
    /* Reconstruct download from handle */
    UplinkDownload download = { ._handle = work_data->download_handle };
    
    /*
     * Single call to uplink_download_read — matching the old uplink-nodejs pattern.
     * Go's io.Reader.Read() may return fewer bytes than requested (partial reads).
     * The JavaScript caller is responsible for looping until all bytes are read.
     * EOF is returned as an error (error->code == EOF) which the complete handler
     * will reject, letting the JS caller catch it and break its read loop.
     */
    uint8_t* buf = (uint8_t*)work_data->buffer_ptr;
    work_data->result = uplink_download_read(&download, buf, work_data->data_length);
    
    LOG_DEBUG("download_read_execute: bytes_read=%zu, error=%s",
              work_data->result.bytes_read,
              work_data->result.error ? (work_data->result.error->message ? work_data->result.error->message : "(EOF)") : "none");
}

/* ========== download_info execute ========== */

void download_info_execute(napi_env env, void* data) {
    (void)env;
    DownloadInfoData* work_data = (DownloadInfoData*)data;
    
    LOG_DEBUG("download_info_execute: handle=%zu", work_data->download_handle);
    
    /* Reconstruct download from handle */
    UplinkDownload download = { ._handle = work_data->download_handle };
    
    /* Call uplink-c */
    work_data->result = uplink_download_info(&download);
    
    if (work_data->result.error) {
        LOG_ERROR("download_info_execute failed: %s", work_data->result.error->message);
    } else {
        LOG_DEBUG("download_info_execute success: key=%s", work_data->result.object ? work_data->result.object->key : "(null)");
    }
}

/* ========== close_download execute ========== */

void close_download_execute(napi_env env, void* data) {
    (void)env;
    CloseDownloadData* work_data = (CloseDownloadData*)data;
    
    LOG_DEBUG("close_download_execute: handle=%zu", work_data->download_handle);
    
    /* Reconstruct download from handle */
    UplinkDownload download = { ._handle = work_data->download_handle };
    
    /* Call uplink-c */
    work_data->error = uplink_close_download(&download);
    
    if (work_data->error) {
        LOG_ERROR("close_download_execute failed: %s", work_data->error->message);
    } else {
        LOG_DEBUG("close_download_execute success");
    }
}
