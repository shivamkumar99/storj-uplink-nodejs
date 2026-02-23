/**
 * @file multipart_ops.c
 * @brief N-API entry points for multipart upload operations
 * 
 * This file contains the public N-API functions that are exposed to JavaScript.
 * These functions:
 * - Extract and validate arguments from JavaScript
 * - Allocate and initialize async work data
 * - Create promises and queue async work
 * 
 * The actual work is done in:
 * - multipart_execute.c (worker thread functions)
 * - multipart_complete.c (main thread completion handlers)
 */

#include "multipart_ops.h"
#include "multipart_types.h"
#include "multipart_execute.h"
#include "multipart_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/type_converters.h"
#include "../common/object_converter.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== begin_upload ========== */

napi_value begin_upload(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 3) {
        napi_throw_type_error(env, NULL, "projectHandle, bucket, and key are required");
        return NULL;
    }
    
    /* Extract project handle */
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    /* Extract bucket name and object key */
    char* bucket_name = NULL;
    char* object_key = NULL;
    
    status = extract_string_required(env, argv[1], "bucket", &bucket_name);
    if (status != napi_ok) {
        return NULL;
    }
    
    status = extract_string_required(env, argv[2], "key", &object_key);
    if (status != napi_ok) {
        free(bucket_name);
        return NULL;
    }
    
    /* Extract optional options */
    int64_t expires = 0;
    if (argc >= 4) {
        napi_valuetype type;
        napi_typeof(env, argv[3], &type);
        if (type == napi_object) {
            expires = get_int64_property(env, argv[3], "expires", 0);
        }
    }
    
    LOG_DEBUG("beginUpload: queuing async work for '%s/%s'", bucket_name, object_key);
    
    BeginUploadData* work_data = (BeginUploadData*)calloc(1, sizeof(BeginUploadData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->expires = expires;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "beginUpload", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        begin_upload_execute,
        begin_upload_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== commit_upload ========== */

napi_value commit_upload(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 4) {
        napi_throw_type_error(env, NULL, "projectHandle, bucket, key, and uploadId are required");
        return NULL;
    }
    
    /* Extract project handle */
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    /* Extract strings */
    char* bucket_name = NULL;
    char* object_key = NULL;
    char* upload_id = NULL;
    
    status = extract_string_required(env, argv[1], "bucket", &bucket_name);
    if (status != napi_ok) return NULL;
    
    status = extract_string_required(env, argv[2], "key", &object_key);
    if (status != napi_ok) {
        free(bucket_name);
        return NULL;
    }
    
    status = extract_string_required(env, argv[3], "uploadId", &upload_id);
    if (status != napi_ok) {
        free(bucket_name);
        free(object_key);
        return NULL;
    }
    
    /* Extract optional custom metadata */
    UplinkCustomMetadataEntry* metadata_entries = NULL;
    size_t metadata_count = 0;
    
    if (argc >= 5) {
        napi_valuetype type;
        napi_typeof(env, argv[4], &type);
        if (type == napi_object) {
            napi_value custom_metadata_val;
            if (napi_get_named_property(env, argv[4], "customMetadata", &custom_metadata_val) == napi_ok) {
                napi_typeof(env, custom_metadata_val, &type);
                if (type == napi_object) {
                    int meta_rc = extract_metadata_entries_from_js(
                        env, custom_metadata_val, &metadata_entries, &metadata_count);
                    if (meta_rc == -1) {
                        free(bucket_name);
                        free(object_key);
                        free(upload_id);
                        napi_throw_type_error(env, NULL, "metadata values must be strings");
                        return NULL;
                    }
                    if (meta_rc == -2) {
                        free(bucket_name);
                        free(object_key);
                        free(upload_id);
                        napi_throw_error(env, NULL, "Out of memory allocating metadata entries");
                        return NULL;
                    }
                }
            }
        }
    }
    
    LOG_DEBUG("commitUpload: queuing async work for '%s/%s' uploadId='%s'", 
              bucket_name, object_key, upload_id);
    
    CommitUploadData* work_data = (CommitUploadData*)calloc(1, sizeof(CommitUploadData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        free(upload_id);
        free_metadata_entries(metadata_entries, metadata_count);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->upload_id = upload_id;
    work_data->metadata_entries = metadata_entries;
    work_data->metadata_count = metadata_count;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "commitUpload", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        commit_upload_execute,
        commit_upload_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== abort_upload ========== */

napi_value abort_upload(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 4) {
        napi_throw_type_error(env, NULL, "projectHandle, bucket, key, and uploadId are required");
        return NULL;
    }
    
    /* Extract project handle */
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    /* Extract strings */
    char* bucket_name = NULL;
    char* object_key = NULL;
    char* upload_id = NULL;
    
    status = extract_string_required(env, argv[1], "bucket", &bucket_name);
    if (status != napi_ok) return NULL;
    
    status = extract_string_required(env, argv[2], "key", &object_key);
    if (status != napi_ok) {
        free(bucket_name);
        return NULL;
    }
    
    status = extract_string_required(env, argv[3], "uploadId", &upload_id);
    if (status != napi_ok) {
        free(bucket_name);
        free(object_key);
        return NULL;
    }
    
    LOG_DEBUG("abortUpload: queuing async work for '%s/%s' uploadId='%s'", 
              bucket_name, object_key, upload_id);
    
    AbortUploadData* work_data = (AbortUploadData*)calloc(1, sizeof(AbortUploadData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        free(upload_id);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->upload_id = upload_id;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "abortUpload", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        abort_upload_execute,
        abort_upload_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== upload_part ========== */

napi_value upload_part(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 5) {
        napi_throw_type_error(env, NULL, "projectHandle, bucket, key, uploadId, and partNumber are required");
        return NULL;
    }
    
    /* Extract project handle */
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    /* Extract strings */
    char* bucket_name = NULL;
    char* object_key = NULL;
    char* upload_id = NULL;
    
    status = extract_string_required(env, argv[1], "bucket", &bucket_name);
    if (status != napi_ok) return NULL;
    
    status = extract_string_required(env, argv[2], "key", &object_key);
    if (status != napi_ok) {
        free(bucket_name);
        return NULL;
    }
    
    status = extract_string_required(env, argv[3], "uploadId", &upload_id);
    if (status != napi_ok) {
        free(bucket_name);
        free(object_key);
        return NULL;
    }
    
    /* Extract part number */
    napi_valuetype pn_type;
    napi_typeof(env, argv[4], &pn_type);
    if (pn_type != napi_number) {
        free(bucket_name);
        free(object_key);
        free(upload_id);
        napi_throw_type_error(env, NULL, "partNumber must be a number");
        return NULL;
    }
    
    uint32_t part_number;
    napi_get_value_uint32(env, argv[4], &part_number);
    
    LOG_DEBUG("uploadPart: queuing async work for part %u of '%s/%s' uploadId='%s'", 
              part_number, bucket_name, object_key, upload_id);
    
    UploadPartData* work_data = (UploadPartData*)calloc(1, sizeof(UploadPartData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        free(upload_id);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->upload_id = upload_id;
    work_data->part_number = part_number;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadPart", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        upload_part_execute,
        upload_part_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== part_upload_write ========== */

napi_value part_upload_write(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 3) {
        napi_throw_type_error(env, NULL, "partUploadHandle, buffer, and length are required");
        return NULL;
    }
    
    /* Extract part upload handle */
    size_t part_upload_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_UPLOAD, &part_upload_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part upload handle");
        return NULL;
    }
    
    /* Extract buffer */
    bool is_buffer;
    napi_is_buffer(env, argv[1], &is_buffer);
    if (!is_buffer) {
        napi_throw_type_error(env, NULL, "buffer must be a Buffer");
        return NULL;
    }
    
    void* buffer;
    size_t buffer_length;
    napi_get_buffer_info(env, argv[1], &buffer, &buffer_length);
    
    /* Extract length */
    napi_valuetype length_type;
    napi_typeof(env, argv[2], &length_type);
    if (length_type != napi_number) {
        napi_throw_type_error(env, NULL, "length must be a number");
        return NULL;
    }
    
    int64_t length;
    napi_get_value_int64(env, argv[2], &length);
    
    if (length < 0 || (size_t)length > buffer_length) {
        napi_throw_range_error(env, NULL, "length out of range");
        return NULL;
    }
    
    LOG_DEBUG("partUploadWrite: queuing async work to write %lld bytes", (long long)length);
    
    PartUploadWriteData* work_data = (PartUploadWriteData*)calloc(1, sizeof(PartUploadWriteData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->part_upload_handle = part_upload_handle;
    work_data->buffer = buffer;
    work_data->length = (size_t)length;
    
    /* Keep buffer alive during async operation */
    napi_create_reference(env, argv[1], 1, &work_data->buffer_ref);
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partUploadWrite", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_upload_write_execute,
        part_upload_write_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== part_upload_commit ========== */

napi_value part_upload_commit(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "partUploadHandle is required");
        return NULL;
    }
    
    /* Extract part upload handle */
    size_t part_upload_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_UPLOAD, &part_upload_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part upload handle");
        return NULL;
    }
    
    LOG_DEBUG("partUploadCommit: queuing async work");
    
    PartUploadOpData* work_data = (PartUploadOpData*)calloc(1, sizeof(PartUploadOpData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->part_upload_handle = part_upload_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partUploadCommit", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_upload_commit_execute,
        part_upload_commit_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== part_upload_abort ========== */

napi_value part_upload_abort(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "partUploadHandle is required");
        return NULL;
    }
    
    /* Extract part upload handle */
    size_t part_upload_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_UPLOAD, &part_upload_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part upload handle");
        return NULL;
    }
    
    LOG_DEBUG("partUploadAbort: queuing async work");
    
    PartUploadOpData* work_data = (PartUploadOpData*)calloc(1, sizeof(PartUploadOpData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->part_upload_handle = part_upload_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partUploadAbort", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_upload_abort_execute,
        part_upload_abort_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== part_upload_set_etag ========== */

napi_value part_upload_set_etag(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "partUploadHandle and etag are required");
        return NULL;
    }
    
    /* Extract part upload handle */
    size_t part_upload_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_UPLOAD, &part_upload_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part upload handle");
        return NULL;
    }
    
    /* Extract etag */
    char* etag = NULL;
    status = extract_string_required(env, argv[1], "etag", &etag);
    if (status != napi_ok) {
        return NULL;
    }
    
    LOG_DEBUG("partUploadSetEtag: queuing async work for etag='%s'", etag);
    
    PartUploadSetEtagData* work_data = (PartUploadSetEtagData*)calloc(1, sizeof(PartUploadSetEtagData));
    if (work_data == NULL) {
        free(etag);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->part_upload_handle = part_upload_handle;
    work_data->etag = etag;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partUploadSetEtag", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_upload_set_etag_execute,
        part_upload_set_etag_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== part_upload_info ========== */

napi_value part_upload_info(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "partUploadHandle is required");
        return NULL;
    }
    
    /* Extract part upload handle */
    size_t part_upload_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_UPLOAD, &part_upload_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part upload handle");
        return NULL;
    }
    
    LOG_DEBUG("partUploadInfo: queuing async work");
    
    PartUploadInfoData* work_data = (PartUploadInfoData*)calloc(1, sizeof(PartUploadInfoData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->part_upload_handle = part_upload_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partUploadInfo", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_upload_info_execute,
        part_upload_info_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== listUploadPartsCreate ========== */

napi_value list_upload_parts_create(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value argv[5];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 4) {
        napi_throw_type_error(env, NULL, "projectHandle, bucket, key, and uploadId are required");
        return NULL;
    }
    
    /* Extract project handle */
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    /* Extract strings */
    char* bucket_name = NULL;
    char* object_key = NULL;
    char* upload_id = NULL;
    
    status = extract_string_required(env, argv[1], "bucket", &bucket_name);
    if (status != napi_ok) return NULL;
    
    status = extract_string_required(env, argv[2], "key", &object_key);
    if (status != napi_ok) {
        free(bucket_name);
        return NULL;
    }
    
    status = extract_string_required(env, argv[3], "uploadId", &upload_id);
    if (status != napi_ok) {
        free(bucket_name);
        free(object_key);
        return NULL;
    }
    
    /* Extract optional cursor */
    uint32_t cursor = 0;
    if (argc >= 5) {
        napi_valuetype type;
        napi_typeof(env, argv[4], &type);
        if (type == napi_object) {
            cursor = (uint32_t)get_int64_property(env, argv[4], "cursor", 0);
        }
    }
    
    LOG_DEBUG("listUploadPartsCreate: queuing async work for '%s/%s' uploadId='%s'", 
              bucket_name, object_key, upload_id);
    
    ListUploadPartsCreateData* work_data = (ListUploadPartsCreateData*)calloc(1, sizeof(ListUploadPartsCreateData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        free(upload_id);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->upload_id = upload_id;
    work_data->cursor = cursor;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "listUploadPartsCreate", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        list_upload_parts_create_execute,
        list_upload_parts_create_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== partIteratorNext ========== */

napi_value part_iterator_next(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part iterator handle");
        return NULL;
    }
    
    PartIteratorNextData* work_data = (PartIteratorNextData*)calloc(1, sizeof(PartIteratorNextData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partIteratorNext", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_iterator_next_execute,
        part_iterator_next_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== partIteratorItem ========== */

napi_value part_iterator_item(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part iterator handle");
        return NULL;
    }
    
    PartIteratorItemData* work_data = (PartIteratorItemData*)calloc(1, sizeof(PartIteratorItemData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partIteratorItem", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_iterator_item_execute,
        part_iterator_item_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== partIteratorErr ========== */

napi_value part_iterator_err(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part iterator handle");
        return NULL;
    }
    
    PartIteratorErrData* work_data = (PartIteratorErrData*)calloc(1, sizeof(PartIteratorErrData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "partIteratorErr", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        part_iterator_err_execute,
        part_iterator_err_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== freePartIterator ========== */

napi_value free_part_iterator(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PART_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid part iterator handle");
        return NULL;
    }
    
    FreePartIteratorData* work_data = (FreePartIteratorData*)calloc(1, sizeof(FreePartIteratorData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "freePartIterator", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        free_part_iterator_execute,
        free_part_iterator_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== listUploadsCreate ========== */

napi_value list_uploads_create(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "projectHandle and bucket are required");
        return NULL;
    }
    
    /* Extract project handle */
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    /* Extract bucket name */
    char* bucket_name = NULL;
    status = extract_string_required(env, argv[1], "bucket", &bucket_name);
    if (status != napi_ok) return NULL;
    
    /* Extract optional options */
    char* prefix = NULL;
    char* cursor = NULL;
    bool recursive = false;
    bool include_system = true;
    bool include_custom = false;
    
    if (argc >= 3) {
        napi_valuetype type;
        napi_typeof(env, argv[2], &type);
        if (type == napi_object) {
            prefix = get_string_property(env, argv[2], "prefix");
            cursor = get_string_property(env, argv[2], "cursor");
            recursive = get_bool_property(env, argv[2], "recursive", 0);
            include_system = get_bool_property(env, argv[2], "system", 1);
            include_custom = get_bool_property(env, argv[2], "custom", 0);
        }
    }
    
    LOG_DEBUG("listUploadsCreate: queuing async work for '%s'", bucket_name);
    
    ListUploadsCreateData* work_data = (ListUploadsCreateData*)calloc(1, sizeof(ListUploadsCreateData));
    if (work_data == NULL) {
        free(bucket_name);
        free(prefix);
        free(cursor);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->prefix = prefix;
    work_data->cursor = cursor;
    work_data->recursive = recursive;
    work_data->include_system = include_system;
    work_data->include_custom = include_custom;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "listUploadsCreate", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        list_uploads_create_execute,
        list_uploads_create_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== uploadIteratorNext ========== */

napi_value upload_iterator_next(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid upload iterator handle");
        return NULL;
    }
    
    UploadIteratorNextData* work_data = (UploadIteratorNextData*)calloc(1, sizeof(UploadIteratorNextData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadIteratorNext", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        upload_iterator_next_execute,
        upload_iterator_next_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== uploadIteratorItem ========== */

napi_value upload_iterator_item(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid upload iterator handle");
        return NULL;
    }
    
    UploadIteratorItemData* work_data = (UploadIteratorItemData*)calloc(1, sizeof(UploadIteratorItemData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadIteratorItem", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        upload_iterator_item_execute,
        upload_iterator_item_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== uploadIteratorErr ========== */

napi_value upload_iterator_err(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid upload iterator handle");
        return NULL;
    }
    
    UploadIteratorErrData* work_data = (UploadIteratorErrData*)calloc(1, sizeof(UploadIteratorErrData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "uploadIteratorErr", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        upload_iterator_err_execute,
        upload_iterator_err_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== freeUploadIterator ========== */

napi_value free_upload_iterator(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_UPLOAD_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid upload iterator handle");
        return NULL;
    }
    
    FreeUploadIteratorData* work_data = (FreeUploadIteratorData*)calloc(1, sizeof(FreeUploadIteratorData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "freeUploadIterator", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        free_upload_iterator_execute,
        free_upload_iterator_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}
