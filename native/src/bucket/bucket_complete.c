/**
 * @file bucket_complete.c
 * @brief Complete function implementations for bucket async operations
 * 
 * These functions run on the main thread and handle promise resolution/rejection.
 */

#include "bucket_complete.h"
#include "bucket_types.h"
#include "../common/handle_helpers.h"
#include "../common/result_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== Helper: Convert Bucket to JS Object ========== */

napi_value uplink_bucket_to_js(napi_env env, UplinkBucket* bucket) {
    if (bucket == NULL) {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        return undefined;
    }

    napi_value obj;
    napi_create_object(env, &obj);

    /* name */
    napi_value name;
    napi_create_string_utf8(env, bucket->name, NAPI_AUTO_LENGTH, &name);
    napi_set_named_property(env, obj, "name", name);

    /* created - Unix timestamp (seconds) */
    napi_value created;
    napi_create_int64(env, bucket->created, &created);
    napi_set_named_property(env, obj, "created", created);

    return obj;
}

/* ========== createBucket complete ========== */

void create_bucket_complete(napi_env env, napi_status status, void* data) {
    BucketOpData* work_data = (BucketOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "createBucket");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("createBucket: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value bucket_obj = uplink_bucket_to_js(env, work_data->result.bucket);
    uplink_free_bucket_result(work_data->result);
    
    LOG_INFO("createBucket: successfully created bucket '%s'", work_data->bucket_name);
    napi_resolve_deferred(env, work_data->deferred, bucket_obj);
    
cleanup:
    free(work_data->bucket_name);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== ensureBucket complete ========== */

void ensure_bucket_complete(napi_env env, napi_status status, void* data) {
    BucketOpData* work_data = (BucketOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "ensureBucket");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("ensureBucket: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value bucket_obj = uplink_bucket_to_js(env, work_data->result.bucket);
    uplink_free_bucket_result(work_data->result);
    
    LOG_INFO("ensureBucket: bucket '%s' ensured", work_data->bucket_name);
    napi_resolve_deferred(env, work_data->deferred, bucket_obj);
    
cleanup:
    free(work_data->bucket_name);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== statBucket complete ========== */

void stat_bucket_complete(napi_env env, napi_status status, void* data) {
    BucketOpData* work_data = (BucketOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "statBucket");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("statBucket: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value bucket_obj = uplink_bucket_to_js(env, work_data->result.bucket);
    uplink_free_bucket_result(work_data->result);
    
    LOG_INFO("statBucket: got info for bucket '%s'", work_data->bucket_name);
    napi_resolve_deferred(env, work_data->deferred, bucket_obj);
    
cleanup:
    free(work_data->bucket_name);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== deleteBucket complete ========== */

void delete_bucket_complete(napi_env env, napi_status status, void* data) {
    BucketOpData* work_data = (BucketOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "deleteBucket");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("deleteBucket: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    /* Free the bucket if returned */
    if (work_data->result.bucket != NULL) {
        uplink_free_bucket(work_data->result.bucket);
    }
    
    LOG_INFO("deleteBucket: successfully deleted bucket '%s'", work_data->bucket_name);
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    free(work_data->bucket_name);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== deleteBucketWithObjects complete ========== */

void delete_bucket_with_objects_complete(napi_env env, napi_status status, void* data) {
    BucketOpData* work_data = (BucketOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "deleteBucketWithObjects");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("deleteBucketWithObjects: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    /* Free the bucket if returned */
    if (work_data->result.bucket != NULL) {
        uplink_free_bucket(work_data->result.bucket);
    }
    
    LOG_INFO("deleteBucketWithObjects: successfully deleted bucket '%s' with objects", 
             work_data->bucket_name);
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    free(work_data->bucket_name);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== listBucketsCreate complete ========== */

void list_buckets_create_complete(napi_env env, napi_status status, void* data) {
    ListBucketsCreateData* work_data = (ListBucketsCreateData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "listBucketsCreate");
    
    if (work_data->iterator_handle == 0) {
        LOG_ERROR("listBucketsCreate: failed to create iterator");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create bucket iterator", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    /* Create handle external for the iterator */
    napi_value handle_obj = create_handle_external(env, work_data->iterator_handle, 
                                                    HANDLE_TYPE_BUCKET_ITERATOR, NULL, NULL);
    
    LOG_INFO("listBucketsCreate: iterator created, handle=%zu", work_data->iterator_handle);
    napi_resolve_deferred(env, work_data->deferred, handle_obj);
    
cleanup:
    free(work_data->cursor);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== bucketIteratorNext complete ========== */

void bucket_iterator_next_complete(napi_env env, napi_status status, void* data) {
    BucketIteratorNextData* work_data = (BucketIteratorNextData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "bucketIteratorNext");
    
    napi_value result;
    napi_get_boolean(env, work_data->has_next, &result);
    napi_resolve_deferred(env, work_data->deferred, result);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== bucketIteratorItem complete ========== */

void bucket_iterator_item_complete(napi_env env, napi_status status, void* data) {
    BucketIteratorItemData* work_data = (BucketIteratorItemData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "bucketIteratorItem");
    
    napi_value bucket_obj = uplink_bucket_to_js(env, work_data->bucket);
    
    if (work_data->bucket != NULL) {
        uplink_free_bucket(work_data->bucket);
    }
    
    LOG_DEBUG("bucketIteratorItem: returned bucket item");
    napi_resolve_deferred(env, work_data->deferred, bucket_obj);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== bucketIteratorErr complete ========== */

void bucket_iterator_err_complete(napi_env env, napi_status status, void* data) {
    BucketIteratorErrData* work_data = (BucketIteratorErrData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "bucketIteratorErr");
    
    if (work_data->error != NULL) {
        LOG_ERROR("bucketIteratorErr: iteration error - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        uplink_free_error(work_data->error);
        /* Resolve with the error object (not reject) — JS decides what to do */
        napi_resolve_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    /* No error — resolve with null */
    napi_value null_val;
    napi_get_null(env, &null_val);
    napi_resolve_deferred(env, work_data->deferred, null_val);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== freeBucketIterator complete ========== */

void free_bucket_iterator_complete(napi_env env, napi_status status, void* data) {
    FreeBucketIteratorData* work_data = (FreeBucketIteratorData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "freeBucketIterator");
    
    LOG_INFO("freeBucketIterator: iterator freed");
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
