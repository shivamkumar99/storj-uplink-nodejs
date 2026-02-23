/**
 * @file object_ops.c
 * @brief N-API entry points for object operations
 * 
 * This file contains the public N-API functions that are exposed to JavaScript.
 * The actual work is done in:
 * - object_execute.c (worker thread functions)
 * - object_complete.c (main thread completion handlers)
 */

#include "object_ops.h"
#include "object_types.h"
#include "object_execute.h"
#include "object_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/type_converters.h"
#include "../common/object_converter.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== stat_object ========== */

napi_value stat_object(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
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
    
    LOG_DEBUG("statObject: queuing async work for '%s/%s'", bucket_name, object_key);
    
    ObjectOpData* work_data = (ObjectOpData*)calloc(1, sizeof(ObjectOpData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "statObject", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        stat_object_execute,
        stat_object_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== delete_object ========== */

napi_value delete_object(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 3) {
        napi_throw_type_error(env, NULL, "projectHandle, bucket, and key are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
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
    
    LOG_DEBUG("deleteObject: queuing async work for '%s/%s'", bucket_name, object_key);
    
    ObjectOpData* work_data = (ObjectOpData*)calloc(1, sizeof(ObjectOpData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "deleteObject", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        delete_object_execute,
        delete_object_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== listObjectsCreate ========== */

napi_value list_objects_create(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "projectHandle and bucket are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    char* bucket_name = NULL;
    status = extract_string_required(env, argv[1], "bucket", &bucket_name);
    if (status != napi_ok) {
        return NULL;
    }
    
    LOG_DEBUG("listObjectsCreate: queuing async work for bucket '%s'", bucket_name);
    
    ListObjectsCreateData* work_data = (ListObjectsCreateData*)calloc(1, sizeof(ListObjectsCreateData));
    if (work_data == NULL) {
        free(bucket_name);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->prefix = NULL;
    work_data->cursor = NULL;
    work_data->recursive = false;
    work_data->include_system = true;
    work_data->include_custom = false;
    work_data->iterator_handle = 0;
    
    /* Parse options if provided */
    if (argc >= 3) {
        napi_valuetype type;
        napi_typeof(env, argv[2], &type);
        if (type == napi_object) {
            work_data->prefix = get_string_property(env, argv[2], "prefix");
            work_data->cursor = get_string_property(env, argv[2], "cursor");
            work_data->recursive = get_bool_property(env, argv[2], "recursive", 0);
            work_data->include_system = get_bool_property(env, argv[2], "system", 1);
            work_data->include_custom = get_bool_property(env, argv[2], "custom", 0);
        }
    }
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "listObjectsCreate", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        list_objects_create_execute,
        list_objects_create_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== objectIteratorNext ========== */

napi_value object_iterator_next(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_OBJECT_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid object iterator handle");
        return NULL;
    }
    
    ObjectIteratorNextData* work_data = (ObjectIteratorNextData*)calloc(1, sizeof(ObjectIteratorNextData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    work_data->has_next = false;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "objectIteratorNext", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(env, NULL, work_name,
        object_iterator_next_execute, object_iterator_next_complete,
        work_data, &work_data->work);
    
    napi_queue_async_work(env, work_data->work);
    return promise;
}

/* ========== objectIteratorItem ========== */

napi_value object_iterator_item(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_OBJECT_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid object iterator handle");
        return NULL;
    }
    
    ObjectIteratorItemData* work_data = (ObjectIteratorItemData*)calloc(1, sizeof(ObjectIteratorItemData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    work_data->object = NULL;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "objectIteratorItem", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(env, NULL, work_name,
        object_iterator_item_execute, object_iterator_item_complete,
        work_data, &work_data->work);
    
    napi_queue_async_work(env, work_data->work);
    return promise;
}

/* ========== objectIteratorErr ========== */

napi_value object_iterator_err(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_OBJECT_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid object iterator handle");
        return NULL;
    }
    
    ObjectIteratorErrData* work_data = (ObjectIteratorErrData*)calloc(1, sizeof(ObjectIteratorErrData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    work_data->error = NULL;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "objectIteratorErr", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(env, NULL, work_name,
        object_iterator_err_execute, object_iterator_err_complete,
        work_data, &work_data->work);
    
    napi_queue_async_work(env, work_data->work);
    return promise;
}

/* ========== freeObjectIterator ========== */

napi_value free_object_iterator(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_OBJECT_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid object iterator handle");
        return NULL;
    }
    
    FreeObjectIteratorData* work_data = (FreeObjectIteratorData*)calloc(1, sizeof(FreeObjectIteratorData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "freeObjectIterator", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(env, NULL, work_name,
        free_object_iterator_execute, free_object_iterator_complete,
        work_data, &work_data->work);
    
    napi_queue_async_work(env, work_data->work);
    return promise;
}

/* ========== copy_object ========== */

napi_value copy_object(napi_env env, napi_callback_info info) {
    size_t argc = 6;
    napi_value argv[6];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 5) {
        napi_throw_type_error(env, NULL, "projectHandle, srcBucket, srcKey, dstBucket, and dstKey are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    char* src_bucket = NULL;
    char* src_key = NULL;
    char* dst_bucket = NULL;
    char* dst_key = NULL;
    
    if (extract_string_required(env, argv[1], "srcBucket", &src_bucket) != napi_ok) {
        return NULL;
    }
    
    if (extract_string_required(env, argv[2], "srcKey", &src_key) != napi_ok) {
        free(src_bucket);
        return NULL;
    }
    
    if (extract_string_required(env, argv[3], "dstBucket", &dst_bucket) != napi_ok) {
        free(src_bucket);
        free(src_key);
        return NULL;
    }
    
    if (extract_string_required(env, argv[4], "dstKey", &dst_key) != napi_ok) {
        free(src_bucket);
        free(src_key);
        free(dst_bucket);
        return NULL;
    }
    
    LOG_DEBUG("copyObject: queuing async work for '%s/%s' -> '%s/%s'", 
              src_bucket, src_key, dst_bucket, dst_key);
    
    CopyMoveObjectData* work_data = (CopyMoveObjectData*)calloc(1, sizeof(CopyMoveObjectData));
    if (work_data == NULL) {
        free(src_bucket);
        free(src_key);
        free(dst_bucket);
        free(dst_key);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->src_bucket = src_bucket;
    work_data->src_key = src_key;
    work_data->dst_bucket = dst_bucket;
    work_data->dst_key = dst_key;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "copyObject", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        copy_object_execute,
        copy_object_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== move_object ========== */

napi_value move_object(napi_env env, napi_callback_info info) {
    size_t argc = 6;
    napi_value argv[6];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 5) {
        napi_throw_type_error(env, NULL, "projectHandle, srcBucket, srcKey, dstBucket, and dstKey are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    char* src_bucket = NULL;
    char* src_key = NULL;
    char* dst_bucket = NULL;
    char* dst_key = NULL;
    
    if (extract_string_required(env, argv[1], "srcBucket", &src_bucket) != napi_ok) {
        return NULL;
    }
    
    if (extract_string_required(env, argv[2], "srcKey", &src_key) != napi_ok) {
        free(src_bucket);
        return NULL;
    }
    
    if (extract_string_required(env, argv[3], "dstBucket", &dst_bucket) != napi_ok) {
        free(src_bucket);
        free(src_key);
        return NULL;
    }
    
    if (extract_string_required(env, argv[4], "dstKey", &dst_key) != napi_ok) {
        free(src_bucket);
        free(src_key);
        free(dst_bucket);
        return NULL;
    }
    
    LOG_DEBUG("moveObject: queuing async work for '%s/%s' -> '%s/%s'", 
              src_bucket, src_key, dst_bucket, dst_key);
    
    CopyMoveObjectData* work_data = (CopyMoveObjectData*)calloc(1, sizeof(CopyMoveObjectData));
    if (work_data == NULL) {
        free(src_bucket);
        free(src_key);
        free(dst_bucket);
        free(dst_key);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->src_bucket = src_bucket;
    work_data->src_key = src_key;
    work_data->dst_bucket = dst_bucket;
    work_data->dst_key = dst_key;
    work_data->move_error = NULL;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "moveObject", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        move_object_execute,
        move_object_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== update_object_metadata ========== */

napi_value update_object_metadata(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value argv[4];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 4) {
        napi_throw_type_error(env, NULL, "projectHandle, bucket, key, and metadata are required");
        return NULL;
    }
    
    /* Extract project handle */
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    /* Extract bucket and key */
    char* bucket_name = NULL;
    char* object_key = NULL;
    
    if (extract_string_required(env, argv[1], "bucket", &bucket_name) != napi_ok) {
        return NULL;
    }
    if (extract_string_required(env, argv[2], "key", &object_key) != napi_ok) {
        free(bucket_name);
        return NULL;
    }
    
    /* Validate metadata type */
    napi_valuetype type;
    napi_typeof(env, argv[3], &type);
    if (type != napi_object) {
        free(bucket_name);
        free(object_key);
        napi_throw_type_error(env, NULL, "metadata must be an object");
        return NULL;
    }
    
    /* Extract metadata entries via helper */
    UplinkCustomMetadataEntry* metadata_entries = NULL;
    size_t metadata_count = 0;
    int meta_rc = extract_metadata_entries_from_js(env, argv[3], &metadata_entries, &metadata_count);
    if (meta_rc == -1) {
        free(bucket_name);
        free(object_key);
        napi_throw_type_error(env, NULL, "metadata values must be strings");
        return NULL;
    }
    if (meta_rc == -2) {
        free(bucket_name);
        free(object_key);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    LOG_DEBUG("updateObjectMetadata: queuing async work for '%s/%s' with %zu entries", 
              bucket_name, object_key, metadata_count);
    
    UpdateMetadataData* work_data = (UpdateMetadataData*)calloc(1, sizeof(UpdateMetadataData));
    if (work_data == NULL) {
        free(bucket_name);
        free(object_key);
        free_metadata_entries(metadata_entries, metadata_count);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    work_data->object_key = object_key;
    work_data->metadata_entries = metadata_entries;
    work_data->metadata_count = metadata_count;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "updateObjectMetadata", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        update_object_metadata_execute,
        update_object_metadata_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}
