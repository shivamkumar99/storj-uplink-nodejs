/**
 * @file multipart_execute.c
 * @brief Worker thread execute functions for multipart upload operations
 * 
 * These functions run on the libuv worker thread and perform the actual
 * uplink-c library calls. They must NOT call any N-API functions.
 */

#include "multipart_execute.h"
#include "multipart_types.h"
#include "../common/buffer_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== begin_upload_execute ========== */

void begin_upload_execute(napi_env env, void* data) {
    (void)env;
    BeginUploadData* work_data = (BeginUploadData*)data;
    
    LOG_DEBUG("beginUpload: starting multipart upload '%s/%s' (worker thread)", 
              work_data->bucket_name, work_data->object_key);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    UplinkUploadOptions* options = NULL;
    UplinkUploadOptions opts = {0};
    if (work_data->expires > 0) {
        opts.expires = work_data->expires;
        options = &opts;
    }
    
    work_data->result = uplink_begin_upload(&project, work_data->bucket_name, work_data->object_key, options);
}

/* ========== commit_upload_execute ========== */

void commit_upload_execute(napi_env env, void* data) {
    (void)env;
    CommitUploadData* work_data = (CommitUploadData*)data;
    
    LOG_DEBUG("commitUpload: committing multipart upload '%s/%s' uploadId='%s' (worker thread)", 
              work_data->bucket_name, work_data->object_key, work_data->upload_id);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    UplinkCommitUploadOptions* options = NULL;
    UplinkCommitUploadOptions opts = {0};
    
    if (work_data->metadata_count > 0) {
        opts.custom_metadata.entries = work_data->metadata_entries;
        opts.custom_metadata.count = work_data->metadata_count;
        options = &opts;
    }
    
    work_data->result = uplink_commit_upload(&project, work_data->bucket_name, 
                                              work_data->object_key, work_data->upload_id, options);
}

/* ========== abort_upload_execute ========== */

void abort_upload_execute(napi_env env, void* data) {
    (void)env;
    AbortUploadData* work_data = (AbortUploadData*)data;
    
    LOG_DEBUG("abortUpload: aborting multipart upload '%s/%s' uploadId='%s' (worker thread)", 
              work_data->bucket_name, work_data->object_key, work_data->upload_id);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->error = uplink_abort_upload(&project, work_data->bucket_name, 
                                            work_data->object_key, work_data->upload_id);
}

/* ========== upload_part_execute ========== */

void upload_part_execute(napi_env env, void* data) {
    (void)env;
    UploadPartData* work_data = (UploadPartData*)data;
    
    LOG_DEBUG("uploadPart: starting part %u for '%s/%s' uploadId='%s' (worker thread)", 
              work_data->part_number, work_data->bucket_name, 
              work_data->object_key, work_data->upload_id);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_upload_part(&project, work_data->bucket_name, 
                                            work_data->object_key, work_data->upload_id,
                                            work_data->part_number);
}

/* ========== part_upload_write_execute ========== */

void part_upload_write_execute(napi_env env, void* data) {
    (void)env;
    PartUploadWriteData* work_data = (PartUploadWriteData*)data;
    
    LOG_DEBUG("partUploadWrite: writing %zu bytes (worker thread)", work_data->length);
    
    UplinkPartUpload part_upload = { ._handle = work_data->part_upload_handle };
    work_data->result = uplink_part_upload_write(&part_upload, work_data->buffer, work_data->length);
}

/* ========== part_upload_commit_execute ========== */

void part_upload_commit_execute(napi_env env, void* data) {
    (void)env;
    PartUploadOpData* work_data = (PartUploadOpData*)data;
    
    LOG_DEBUG("partUploadCommit: committing part (worker thread)");
    
    UplinkPartUpload part_upload = { ._handle = work_data->part_upload_handle };
    work_data->error = uplink_part_upload_commit(&part_upload);
}

/* ========== part_upload_abort_execute ========== */

void part_upload_abort_execute(napi_env env, void* data) {
    (void)env;
    PartUploadOpData* work_data = (PartUploadOpData*)data;
    
    LOG_DEBUG("partUploadAbort: aborting part (worker thread)");
    
    UplinkPartUpload part_upload = { ._handle = work_data->part_upload_handle };
    work_data->error = uplink_part_upload_abort(&part_upload);
}

/* ========== part_upload_set_etag_execute ========== */

void part_upload_set_etag_execute(napi_env env, void* data) {
    (void)env;
    PartUploadSetEtagData* work_data = (PartUploadSetEtagData*)data;
    
    LOG_DEBUG("partUploadSetEtag: setting etag='%s' (worker thread)", work_data->etag);
    
    UplinkPartUpload part_upload = { ._handle = work_data->part_upload_handle };
    work_data->error = uplink_part_upload_set_etag(&part_upload, work_data->etag);
}

/* ========== part_upload_info_execute ========== */

void part_upload_info_execute(napi_env env, void* data) {
    (void)env;
    PartUploadInfoData* work_data = (PartUploadInfoData*)data;
    
    LOG_DEBUG("partUploadInfo: getting part info (worker thread)");
    
    UplinkPartUpload part_upload = { ._handle = work_data->part_upload_handle };
    work_data->result = uplink_part_upload_info(&part_upload);
}

/* ========== listUploadPartsCreate execute ========== */

void list_upload_parts_create_execute(napi_env env, void* data) {
    (void)env;
    ListUploadPartsCreateData* work_data = (ListUploadPartsCreateData*)data;
    
    LOG_DEBUG("listUploadPartsCreate: creating part iterator for '%s/%s' uploadId='%s' (worker thread)", 
              work_data->bucket_name, work_data->object_key, work_data->upload_id);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    UplinkListUploadPartsOptions* options = NULL;
    UplinkListUploadPartsOptions opts = {0};
    if (work_data->cursor > 0) {
        opts.cursor = work_data->cursor;
        options = &opts;
    }
    
    UplinkPartIterator* iterator = uplink_list_upload_parts(&project, work_data->bucket_name, 
                                                            work_data->object_key, work_data->upload_id, options);
    work_data->iterator_handle = (size_t)iterator;
    
    LOG_DEBUG("listUploadPartsCreate: iterator created, handle=%zu", work_data->iterator_handle);
}

/* ========== partIteratorNext execute ========== */

void part_iterator_next_execute(napi_env env, void* data) {
    (void)env;
    PartIteratorNextData* work_data = (PartIteratorNextData*)data;
    
    UplinkPartIterator* iterator = (UplinkPartIterator*)work_data->iterator_handle;
    work_data->has_next = uplink_part_iterator_next(iterator);
    
    LOG_DEBUG("partIteratorNext: has_next=%d", work_data->has_next);
}

/* ========== partIteratorItem execute ========== */

void part_iterator_item_execute(napi_env env, void* data) {
    (void)env;
    PartIteratorItemData* work_data = (PartIteratorItemData*)data;
    
    UplinkPartIterator* iterator = (UplinkPartIterator*)work_data->iterator_handle;
    const UplinkPart* part = uplink_part_iterator_item(iterator);
    
    /* Deep copy the part since iterator items are only valid until next iteration */
    if (part != NULL) {
        work_data->part = (UplinkPart*)calloc(1, sizeof(UplinkPart));
        if (work_data->part != NULL) {
            *work_data->part = *part;
            if (part->etag != NULL && part->etag_length > 0) {
                work_data->part->etag = (char*)malloc(part->etag_length + 1);
                if (work_data->part->etag != NULL) {
                    safe_memcpy(work_data->part->etag, part->etag_length + 1,
                                part->etag, part->etag_length);
                    work_data->part->etag[part->etag_length] = '\0';
                }
            }
        }
    } else {
        work_data->part = NULL;
    }
}

/* ========== partIteratorErr execute ========== */

void part_iterator_err_execute(napi_env env, void* data) {
    (void)env;
    PartIteratorErrData* work_data = (PartIteratorErrData*)data;
    
    UplinkPartIterator* iterator = (UplinkPartIterator*)work_data->iterator_handle;
    work_data->error = uplink_part_iterator_err(iterator);
}

/* ========== freePartIterator execute ========== */

void free_part_iterator_execute(napi_env env, void* data) {
    (void)env;
    FreePartIteratorData* work_data = (FreePartIteratorData*)data;
    
    UplinkPartIterator* iterator = (UplinkPartIterator*)work_data->iterator_handle;
    uplink_free_part_iterator(iterator);
}

/* ========== listUploadsCreate execute ========== */

void list_uploads_create_execute(napi_env env, void* data) {
    (void)env;
    ListUploadsCreateData* work_data = (ListUploadsCreateData*)data;
    
    LOG_DEBUG("listUploadsCreate: creating upload iterator for '%s' (worker thread)", work_data->bucket_name);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    UplinkListUploadsOptions opts = {0};
    opts.prefix = work_data->prefix;
    opts.cursor = work_data->cursor;
    opts.recursive = work_data->recursive;
    opts.system = work_data->include_system;
    opts.custom = work_data->include_custom;
    
    UplinkUploadIterator* iterator = uplink_list_uploads(&project, work_data->bucket_name, &opts);
    work_data->iterator_handle = (size_t)iterator;
    
    LOG_DEBUG("listUploadsCreate: iterator created, handle=%zu", work_data->iterator_handle);
}

/* ========== uploadIteratorNext execute ========== */

void upload_iterator_next_execute(napi_env env, void* data) {
    (void)env;
    UploadIteratorNextData* work_data = (UploadIteratorNextData*)data;
    
    UplinkUploadIterator* iterator = (UplinkUploadIterator*)work_data->iterator_handle;
    work_data->has_next = uplink_upload_iterator_next(iterator);
    
    LOG_DEBUG("uploadIteratorNext: has_next=%d", work_data->has_next);
}

/* ========== helper: deep copy custom metadata ========== */

/**
 * Deep-copy custom metadata entries from an UplinkUploadInfo.
 * The iterator item is only valid until the next iteration, so we
 * must duplicate every string.
 */
static void deep_copy_upload_custom_metadata(UplinkCustomMetadata* dst,
                                             const UplinkCustomMetadata* src) {
    if (src->count == 0 || src->entries == NULL) {
        return;
    }
    dst->count = src->count;
    dst->entries = (UplinkCustomMetadataEntry*)calloc(
        src->count, sizeof(UplinkCustomMetadataEntry));
    if (dst->entries == NULL) {
        dst->count = 0;
        return;
    }
    for (size_t i = 0; i < src->count; i++) {
        dst->entries[i].key = strdup(src->entries[i].key);
        dst->entries[i].key_length = src->entries[i].key_length;
        dst->entries[i].value = strdup(src->entries[i].value);
        dst->entries[i].value_length = src->entries[i].value_length;
    }
}

/* ========== uploadIteratorItem execute ========== */

void upload_iterator_item_execute(napi_env env, void* data) {
    (void)env;
    UploadIteratorItemData* work_data = (UploadIteratorItemData*)data;
    
    UplinkUploadIterator* iterator = (UplinkUploadIterator*)work_data->iterator_handle;
    UplinkUploadInfo* upload = uplink_upload_iterator_item(iterator);
    
    if (upload == NULL) {
        work_data->upload_info = NULL;
        return;
    }

    /* Deep copy since iterator items are only valid until next iteration */
    work_data->upload_info = (UplinkUploadInfo*)calloc(1, sizeof(UplinkUploadInfo));
    if (work_data->upload_info == NULL) {
        return;
    }

    work_data->upload_info->is_prefix = upload->is_prefix;
    work_data->upload_info->system = upload->system;

    if (upload->upload_id != NULL) {
        work_data->upload_info->upload_id = strdup(upload->upload_id);
    }
    if (upload->key != NULL) {
        work_data->upload_info->key = strdup(upload->key);
    }

    deep_copy_upload_custom_metadata(&work_data->upload_info->custom, &upload->custom);
}

/* ========== uploadIteratorErr execute ========== */

void upload_iterator_err_execute(napi_env env, void* data) {
    (void)env;
    UploadIteratorErrData* work_data = (UploadIteratorErrData*)data;
    
    UplinkUploadIterator* iterator = (UplinkUploadIterator*)work_data->iterator_handle;
    work_data->error = uplink_upload_iterator_err(iterator);
}

/* ========== freeUploadIterator execute ========== */

void free_upload_iterator_execute(napi_env env, void* data) {
    (void)env;
    FreeUploadIteratorData* work_data = (FreeUploadIteratorData*)data;
    
    UplinkUploadIterator* iterator = (UplinkUploadIterator*)work_data->iterator_handle;
    uplink_free_upload_iterator(iterator);
}
