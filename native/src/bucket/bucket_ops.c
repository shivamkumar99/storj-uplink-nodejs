/**
 * @file bucket_ops.c
 * @brief N-API entry points for bucket operations
 * 
 * This file contains the N-API entry point functions that:
 * 1. Extract arguments from JavaScript
 * 2. Validate inputs
 * 3. Set up async work
 * 4. Return promises
 */

#include "bucket_ops.h"
#include "bucket_types.h"
#include "bucket_execute.h"
#include "bucket_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== createBucket ========== */

napi_value create_bucket(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "projectHandle and bucketName are required");
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
    status = extract_string_required(env, argv[1], "bucketName", &bucket_name);
    if (status != napi_ok) {
        return NULL;
    }
    
    LOG_DEBUG("createBucket: queuing async work for bucket '%s'", bucket_name);
    
    BucketOpData* work_data = (BucketOpData*)calloc(1, sizeof(BucketOpData));
    if (work_data == NULL) {
        free(bucket_name);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "createBucket", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        create_bucket_execute,
        create_bucket_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== ensureBucket ========== */

napi_value ensure_bucket(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "projectHandle and bucketName are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    char* bucket_name = NULL;
    status = extract_string_required(env, argv[1], "bucketName", &bucket_name);
    if (status != napi_ok) {
        return NULL;
    }
    
    LOG_DEBUG("ensureBucket: queuing async work for bucket '%s'", bucket_name);
    
    BucketOpData* work_data = (BucketOpData*)calloc(1, sizeof(BucketOpData));
    if (work_data == NULL) {
        free(bucket_name);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "ensureBucket", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        ensure_bucket_execute,
        ensure_bucket_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== statBucket ========== */

napi_value stat_bucket(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "projectHandle and bucketName are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    char* bucket_name = NULL;
    status = extract_string_required(env, argv[1], "bucketName", &bucket_name);
    if (status != napi_ok) {
        return NULL;
    }
    
    LOG_DEBUG("statBucket: queuing async work for bucket '%s'", bucket_name);
    
    BucketOpData* work_data = (BucketOpData*)calloc(1, sizeof(BucketOpData));
    if (work_data == NULL) {
        free(bucket_name);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "statBucket", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        stat_bucket_execute,
        stat_bucket_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== deleteBucket ========== */

napi_value delete_bucket(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "projectHandle and bucketName are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    char* bucket_name = NULL;
    status = extract_string_required(env, argv[1], "bucketName", &bucket_name);
    if (status != napi_ok) {
        return NULL;
    }
    
    LOG_DEBUG("deleteBucket: queuing async work for bucket '%s'", bucket_name);
    
    BucketOpData* work_data = (BucketOpData*)calloc(1, sizeof(BucketOpData));
    if (work_data == NULL) {
        free(bucket_name);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "deleteBucket", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        delete_bucket_execute,
        delete_bucket_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== deleteBucketWithObjects ========== */

napi_value delete_bucket_with_objects(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "projectHandle and bucketName are required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    char* bucket_name = NULL;
    status = extract_string_required(env, argv[1], "bucketName", &bucket_name);
    if (status != napi_ok) {
        return NULL;
    }
    
    LOG_DEBUG("deleteBucketWithObjects: queuing async work for bucket '%s'", bucket_name);
    
    BucketOpData* work_data = (BucketOpData*)calloc(1, sizeof(BucketOpData));
    if (work_data == NULL) {
        free(bucket_name);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->bucket_name = bucket_name;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "deleteBucketWithObjects", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        delete_bucket_with_objects_execute,
        delete_bucket_with_objects_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== listBucketsCreate ========== */

napi_value list_buckets_create(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "projectHandle is required");
        return NULL;
    }
    
    size_t project_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    LOG_DEBUG("listBucketsCreate: queuing async work");
    
    ListBucketsCreateData* work_data = (ListBucketsCreateData*)calloc(1, sizeof(ListBucketsCreateData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    work_data->cursor = NULL;
    work_data->iterator_handle = 0;
    
    /* Parse options if provided */
    if (argc >= 2) {
        napi_valuetype type;
        napi_typeof(env, argv[1], &type);
        if (type == napi_object) {
            napi_value cursor_val;
            napi_get_named_property(env, argv[1], "cursor", &cursor_val);
            
            napi_valuetype cursor_type;
            napi_typeof(env, cursor_val, &cursor_type);
            if (cursor_type == napi_string) {
                size_t cursor_len;
                napi_get_value_string_utf8(env, cursor_val, NULL, 0, &cursor_len);
                work_data->cursor = (char*)malloc(cursor_len + 1);
                if (work_data->cursor != NULL) {
                    napi_get_value_string_utf8(env, cursor_val, work_data->cursor, cursor_len + 1, NULL);
                }
            }
        }
    }
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "listBucketsCreate", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        list_buckets_create_execute,
        list_buckets_create_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== bucketIteratorNext ========== */

napi_value bucket_iterator_next(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_BUCKET_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid bucket iterator handle");
        return NULL;
    }
    
    LOG_DEBUG("bucketIteratorNext: queuing async work");
    
    BucketIteratorNextData* work_data = (BucketIteratorNextData*)calloc(1, sizeof(BucketIteratorNextData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    work_data->has_next = false;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "bucketIteratorNext", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        bucket_iterator_next_execute,
        bucket_iterator_next_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== bucketIteratorItem ========== */

napi_value bucket_iterator_item(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_BUCKET_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid bucket iterator handle");
        return NULL;
    }
    
    LOG_DEBUG("bucketIteratorItem: queuing async work");
    
    BucketIteratorItemData* work_data = (BucketIteratorItemData*)calloc(1, sizeof(BucketIteratorItemData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    work_data->bucket = NULL;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "bucketIteratorItem", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        bucket_iterator_item_execute,
        bucket_iterator_item_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== bucketIteratorErr ========== */

napi_value bucket_iterator_err(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_BUCKET_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid bucket iterator handle");
        return NULL;
    }
    
    LOG_DEBUG("bucketIteratorErr: queuing async work");
    
    BucketIteratorErrData* work_data = (BucketIteratorErrData*)calloc(1, sizeof(BucketIteratorErrData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    work_data->error = NULL;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "bucketIteratorErr", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        bucket_iterator_err_execute,
        bucket_iterator_err_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== freeBucketIterator ========== */

napi_value free_bucket_iterator(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "iteratorHandle is required");
        return NULL;
    }
    
    size_t iterator_handle;
    napi_status status = extract_handle(env, argv[0], HANDLE_TYPE_BUCKET_ITERATOR, &iterator_handle);
    if (status != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid bucket iterator handle");
        return NULL;
    }
    
    LOG_DEBUG("freeBucketIterator: queuing async work");
    
    FreeBucketIteratorData* work_data = (FreeBucketIteratorData*)calloc(1, sizeof(FreeBucketIteratorData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->iterator_handle = iterator_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "freeBucketIterator", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        free_bucket_iterator_execute,
        free_bucket_iterator_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}
