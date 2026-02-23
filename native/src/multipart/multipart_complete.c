/**
 * @file multipart_complete.c
 * @brief Main thread completion handlers for multipart upload operations
 * 
 * These functions run on the main thread after the worker thread completes.
 * They handle error checking, result conversion to JavaScript, memory cleanup,
 * and promise resolution/rejection.
 */

#include "multipart_complete.h"
#include "multipart_types.h"
#include "../common/handle_helpers.h"
#include "../common/result_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/object_converter.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== Helper: Convert UploadInfo to JS Object ========== */

static napi_value upload_info_to_js(napi_env env, UplinkUploadInfo* info) {
    if (info == NULL) {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        return undefined;
    }

    napi_value obj;
    napi_create_object(env, &obj);

    /* uploadId */
    napi_value upload_id;
    napi_create_string_utf8(env, info->upload_id ? info->upload_id : "", NAPI_AUTO_LENGTH, &upload_id);
    napi_set_named_property(env, obj, "uploadId", upload_id);

    /* key */
    napi_value key;
    napi_create_string_utf8(env, info->key ? info->key : "", NAPI_AUTO_LENGTH, &key);
    napi_set_named_property(env, obj, "key", key);

    /* isPrefix */
    napi_value is_prefix;
    napi_get_boolean(env, info->is_prefix, &is_prefix);
    napi_set_named_property(env, obj, "isPrefix", is_prefix);

    /* system metadata */
    napi_value system;
    napi_create_object(env, &system);

    napi_value created;
    napi_create_int64(env, info->system.created, &created);
    napi_set_named_property(env, system, "created", created);

    if (info->system.expires != 0) {
        napi_value expires;
        napi_create_int64(env, info->system.expires, &expires);
        napi_set_named_property(env, system, "expires", expires);
    } else {
        napi_value null_val;
        napi_get_null(env, &null_val);
        napi_set_named_property(env, system, "expires", null_val);
    }

    napi_value content_length;
    napi_create_int64(env, info->system.content_length, &content_length);
    napi_set_named_property(env, system, "contentLength", content_length);

    napi_set_named_property(env, obj, "system", system);

    /* custom metadata */
    napi_value custom;
    napi_create_object(env, &custom);

    if (info->custom.count > 0 && info->custom.entries != NULL) {
        for (size_t i = 0; i < info->custom.count; i++) {
            napi_value value;
            napi_create_string_utf8(env, info->custom.entries[i].value, 
                                    info->custom.entries[i].value_length, &value);
            napi_set_named_property(env, custom, info->custom.entries[i].key, value);
        }
    }

    napi_set_named_property(env, obj, "custom", custom);

    return obj;
}

/* ========== Helper: Convert Part to JS Object ========== */

static napi_value part_to_js(napi_env env, UplinkPart* part) {
    if (part == NULL) {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        return undefined;
    }

    napi_value obj;
    napi_create_object(env, &obj);

    /* partNumber */
    napi_value part_number;
    napi_create_uint32(env, part->part_number, &part_number);
    napi_set_named_property(env, obj, "partNumber", part_number);

    /* size */
    napi_value size;
    napi_create_int64(env, (int64_t)part->size, &size);
    napi_set_named_property(env, obj, "size", size);

    /* modified */
    napi_value modified;
    napi_create_int64(env, part->modified, &modified);
    napi_set_named_property(env, obj, "modified", modified);

    /* etag */
    napi_value etag;
    if (part->etag != NULL && part->etag_length > 0) {
        napi_create_string_utf8(env, part->etag, part->etag_length, &etag);
    } else {
        napi_create_string_utf8(env, "", 0, &etag);
    }
    napi_set_named_property(env, obj, "etag", etag);

    return obj;
}

/* ========== begin_upload_complete ========== */

void begin_upload_complete(napi_env env, napi_status status, void* data) {
    BeginUploadData* work_data = (BeginUploadData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "beginUpload");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("beginUpload: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_upload_info_result(work_data->result);
        goto cleanup;
    }
    
    napi_value info_obj = upload_info_to_js(env, work_data->result.info);
    
    LOG_INFO("beginUpload: multipart upload started for '%s/%s', uploadId='%s'", 
             work_data->bucket_name, work_data->object_key, 
             work_data->result.info ? work_data->result.info->upload_id : "");
    
    uplink_free_upload_info_result(work_data->result);
    napi_resolve_deferred(env, work_data->deferred, info_obj);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== commit_upload_complete ========== */

void commit_upload_complete(napi_env env, napi_status status, void* data) {
    CommitUploadData* work_data = (CommitUploadData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "commitUpload");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("commitUpload: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_commit_upload_result(work_data->result);
        goto cleanup;
    }
    
    napi_value obj = uplink_object_to_js(env, work_data->result.object);
    
    LOG_INFO("commitUpload: multipart upload committed for '%s/%s'", 
             work_data->bucket_name, work_data->object_key);
    
    uplink_free_commit_upload_result(work_data->result);
    napi_resolve_deferred(env, work_data->deferred, obj);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    free(work_data->upload_id);
    
    /* Free metadata entries */
    if (work_data->metadata_entries != NULL) {
        for (size_t i = 0; i < work_data->metadata_count; i++) {
            free(work_data->metadata_entries[i].key);
            free(work_data->metadata_entries[i].value);
        }
        free(work_data->metadata_entries);
    }
    
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== abort_upload_complete ========== */

void abort_upload_complete(napi_env env, napi_status status, void* data) {
    AbortUploadData* work_data = (AbortUploadData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "abortUpload");
    
    if (work_data->error != NULL) {
        LOG_ERROR("abortUpload: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    LOG_INFO("abortUpload: multipart upload aborted for '%s/%s'", 
             work_data->bucket_name, work_data->object_key);
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    free(work_data->upload_id);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== upload_part_complete ========== */

void upload_part_complete(napi_env env, napi_status status, void* data) {
    UploadPartData* work_data = (UploadPartData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadPart");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("uploadPart: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_part_upload_result(work_data->result);
        goto cleanup;
    }
    
    /* Create handle for part upload */
    size_t handle = work_data->result.part_upload->_handle;
    napi_value handle_obj = create_handle_external(env, handle, HANDLE_TYPE_PART_UPLOAD, work_data->result.part_upload, NULL);
    
    LOG_INFO("uploadPart: part %u started for '%s/%s', handle=%zu", 
             work_data->part_number, work_data->bucket_name, work_data->object_key, handle);
    
    /* Don't free the result since we're keeping the part upload handle */
    napi_resolve_deferred(env, work_data->deferred, handle_obj);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    free(work_data->upload_id);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== part_upload_write_complete ========== */

void part_upload_write_complete(napi_env env, napi_status status, void* data) {
    PartUploadWriteData* work_data = (PartUploadWriteData*)data;
    
    /* Release buffer reference */
    if (work_data->buffer_ref != NULL) {
        napi_delete_reference(env, work_data->buffer_ref);
    }
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partUploadWrite");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("partUploadWrite: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    LOG_INFO("partUploadWrite: wrote %zu bytes", work_data->result.bytes_written);
    
    napi_value bytes_written;
    napi_create_int64(env, (int64_t)work_data->result.bytes_written, &bytes_written);
    napi_resolve_deferred(env, work_data->deferred, bytes_written);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== part_upload_commit_complete ========== */

void part_upload_commit_complete(napi_env env, napi_status status, void* data) {
    PartUploadOpData* work_data = (PartUploadOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partUploadCommit");
    
    if (work_data->error != NULL) {
        LOG_ERROR("partUploadCommit: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    LOG_INFO("partUploadCommit: part committed");
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== part_upload_abort_complete ========== */

void part_upload_abort_complete(napi_env env, napi_status status, void* data) {
    PartUploadOpData* work_data = (PartUploadOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partUploadAbort");
    
    if (work_data->error != NULL) {
        LOG_ERROR("partUploadAbort: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    LOG_INFO("partUploadAbort: part aborted");
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== part_upload_set_etag_complete ========== */

void part_upload_set_etag_complete(napi_env env, napi_status status, void* data) {
    PartUploadSetEtagData* work_data = (PartUploadSetEtagData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partUploadSetEtag");
    
    if (work_data->error != NULL) {
        LOG_ERROR("partUploadSetEtag: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    LOG_INFO("partUploadSetEtag: etag set");
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    free(work_data->etag);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== part_upload_info_complete ========== */

void part_upload_info_complete(napi_env env, napi_status status, void* data) {
    PartUploadInfoData* work_data = (PartUploadInfoData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partUploadInfo");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("partUploadInfo: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_part_result(work_data->result);
        goto cleanup;
    }
    
    napi_value part_obj = part_to_js(env, work_data->result.part);
    
    LOG_INFO("partUploadInfo: got part info");
    
    uplink_free_part_result(work_data->result);
    napi_resolve_deferred(env, work_data->deferred, part_obj);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== listUploadPartsCreate complete ========== */

void list_upload_parts_create_complete(napi_env env, napi_status status, void* data) {
    ListUploadPartsCreateData* work_data = (ListUploadPartsCreateData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "listUploadPartsCreate");
    
    if (work_data->iterator_handle == 0) {
        LOG_ERROR("listUploadPartsCreate: failed to create iterator");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create part iterator", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    {
        napi_value handle_obj = create_handle_external(env, work_data->iterator_handle, 
                                                        HANDLE_TYPE_PART_ITERATOR, NULL, NULL);
        LOG_INFO("listUploadPartsCreate: iterator created, handle=%zu", work_data->iterator_handle);
        napi_resolve_deferred(env, work_data->deferred, handle_obj);
    }
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    free(work_data->upload_id);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== partIteratorNext complete ========== */

void part_iterator_next_complete(napi_env env, napi_status status, void* data) {
    PartIteratorNextData* work_data = (PartIteratorNextData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partIteratorNext");
    
    {
        napi_value result;
        napi_get_boolean(env, work_data->has_next, &result);
        napi_resolve_deferred(env, work_data->deferred, result);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== partIteratorItem complete ========== */

void part_iterator_item_complete(napi_env env, napi_status status, void* data) {
    PartIteratorItemData* work_data = (PartIteratorItemData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partIteratorItem");
    
    {
        napi_value part_obj = part_to_js(env, work_data->part);
        
        /* Free the deep copy */
        if (work_data->part != NULL) {
            if (work_data->part->etag != NULL) {
                free(work_data->part->etag);
            }
            free(work_data->part);
        }
        
        napi_resolve_deferred(env, work_data->deferred, part_obj);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== partIteratorErr complete ========== */

void part_iterator_err_complete(napi_env env, napi_status status, void* data) {
    PartIteratorErrData* work_data = (PartIteratorErrData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "partIteratorErr");
    
    if (work_data->error != NULL) {
        LOG_ERROR("partIteratorErr: iteration error - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        uplink_free_error(work_data->error);
        napi_resolve_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    {
        napi_value null_val;
        napi_get_null(env, &null_val);
        napi_resolve_deferred(env, work_data->deferred, null_val);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== freePartIterator complete ========== */

void free_part_iterator_complete(napi_env env, napi_status status, void* data) {
    FreePartIteratorData* work_data = (FreePartIteratorData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "freePartIterator");
    
    LOG_INFO("freePartIterator: iterator freed");
    
    {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_resolve_deferred(env, work_data->deferred, undefined);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== listUploadsCreate complete ========== */

void list_uploads_create_complete(napi_env env, napi_status status, void* data) {
    ListUploadsCreateData* work_data = (ListUploadsCreateData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "listUploadsCreate");
    
    if (work_data->iterator_handle == 0) {
        LOG_ERROR("listUploadsCreate: failed to create iterator");
        napi_value error, msg;
        napi_create_string_utf8(env, "Failed to create upload iterator", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    {
        napi_value handle_obj = create_handle_external(env, work_data->iterator_handle, 
                                                        HANDLE_TYPE_UPLOAD_ITERATOR, NULL, NULL);
        LOG_INFO("listUploadsCreate: iterator created, handle=%zu", work_data->iterator_handle);
        napi_resolve_deferred(env, work_data->deferred, handle_obj);
    }
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->prefix);
    free(work_data->cursor);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== uploadIteratorNext complete ========== */

void upload_iterator_next_complete(napi_env env, napi_status status, void* data) {
    UploadIteratorNextData* work_data = (UploadIteratorNextData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadIteratorNext");
    
    {
        napi_value result;
        napi_get_boolean(env, work_data->has_next, &result);
        napi_resolve_deferred(env, work_data->deferred, result);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== uploadIteratorItem complete ========== */

void upload_iterator_item_complete(napi_env env, napi_status status, void* data) {
    UploadIteratorItemData* work_data = (UploadIteratorItemData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadIteratorItem");
    
    {
        napi_value upload_obj = upload_info_to_js(env, work_data->upload_info);
        
        /* Free the deep copy */
        if (work_data->upload_info != NULL) {
            free(work_data->upload_info->upload_id);
            free(work_data->upload_info->key);
            if (work_data->upload_info->custom.entries != NULL) {
                for (size_t j = 0; j < work_data->upload_info->custom.count; j++) {
                    free(work_data->upload_info->custom.entries[j].key);
                    free(work_data->upload_info->custom.entries[j].value);
                }
                free(work_data->upload_info->custom.entries);
            }
            free(work_data->upload_info);
        }
        
        napi_resolve_deferred(env, work_data->deferred, upload_obj);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== uploadIteratorErr complete ========== */

void upload_iterator_err_complete(napi_env env, napi_status status, void* data) {
    UploadIteratorErrData* work_data = (UploadIteratorErrData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "uploadIteratorErr");
    
    if (work_data->error != NULL) {
        LOG_ERROR("uploadIteratorErr: iteration error - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        uplink_free_error(work_data->error);
        napi_resolve_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    {
        napi_value null_val;
        napi_get_null(env, &null_val);
        napi_resolve_deferred(env, work_data->deferred, null_val);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== freeUploadIterator complete ========== */

void free_upload_iterator_complete(napi_env env, napi_status status, void* data) {
    FreeUploadIteratorData* work_data = (FreeUploadIteratorData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "freeUploadIterator");
    
    LOG_INFO("freeUploadIterator: iterator freed");
    
    {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_resolve_deferred(env, work_data->deferred, undefined);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
