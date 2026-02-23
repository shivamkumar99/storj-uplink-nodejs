/**
 * @file bucket_execute.c
 * @brief Execute function implementations for bucket async operations
 * 
 * These functions run on worker threads and call uplink-c functions.
 */

#include "bucket_execute.h"
#include "bucket_types.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== createBucket execute ========== */

void create_bucket_execute(napi_env env, void* data) {
    (void)env;
    BucketOpData* work_data = (BucketOpData*)data;
    
    LOG_DEBUG("createBucket: creating bucket '%s' (worker thread)", work_data->bucket_name);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_create_bucket(&project, work_data->bucket_name);
}

/* ========== ensureBucket execute ========== */

void ensure_bucket_execute(napi_env env, void* data) {
    (void)env;
    BucketOpData* work_data = (BucketOpData*)data;
    
    LOG_DEBUG("ensureBucket: ensuring bucket '%s' (worker thread)", work_data->bucket_name);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_ensure_bucket(&project, work_data->bucket_name);
}

/* ========== statBucket execute ========== */

void stat_bucket_execute(napi_env env, void* data) {
    (void)env;
    BucketOpData* work_data = (BucketOpData*)data;
    
    LOG_DEBUG("statBucket: getting info for bucket '%s' (worker thread)", work_data->bucket_name);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_stat_bucket(&project, work_data->bucket_name);
}

/* ========== deleteBucket execute ========== */

void delete_bucket_execute(napi_env env, void* data) {
    (void)env;
    BucketOpData* work_data = (BucketOpData*)data;
    
    LOG_DEBUG("deleteBucket: deleting bucket '%s' (worker thread)", work_data->bucket_name);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_delete_bucket(&project, work_data->bucket_name);
}

/* ========== deleteBucketWithObjects execute ========== */

void delete_bucket_with_objects_execute(napi_env env, void* data) {
    (void)env;
    BucketOpData* work_data = (BucketOpData*)data;
    
    LOG_DEBUG("deleteBucketWithObjects: deleting bucket '%s' with objects (worker thread)", 
              work_data->bucket_name);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_delete_bucket_with_objects(&project, work_data->bucket_name);
}

/* ========== listBucketsCreate execute ========== */

void list_buckets_create_execute(napi_env env, void* data) {
    (void)env;
    ListBucketsCreateData* work_data = (ListBucketsCreateData*)data;
    
    LOG_DEBUG("listBucketsCreate: creating bucket iterator (worker thread)");
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    /* Prepare options */
    UplinkListBucketsOptions options = { 0 };
    if (work_data->cursor != NULL) {
        options.cursor = work_data->cursor;
    }
    
    UplinkBucketIterator* iterator = uplink_list_buckets(&project, &options);
    work_data->iterator_handle = (size_t)iterator;
    
    LOG_DEBUG("listBucketsCreate: iterator created, handle=%zu", work_data->iterator_handle);
}

/* ========== bucketIteratorNext execute ========== */

void bucket_iterator_next_execute(napi_env env, void* data) {
    (void)env;
    BucketIteratorNextData* work_data = (BucketIteratorNextData*)data;
    
    LOG_DEBUG("bucketIteratorNext: advancing iterator (worker thread)");
    
    UplinkBucketIterator* iterator = (UplinkBucketIterator*)work_data->iterator_handle;
    work_data->has_next = uplink_bucket_iterator_next(iterator);
    
    LOG_DEBUG("bucketIteratorNext: has_next=%d", work_data->has_next);
}

/* ========== bucketIteratorItem execute ========== */

void bucket_iterator_item_execute(napi_env env, void* data) {
    (void)env;
    BucketIteratorItemData* work_data = (BucketIteratorItemData*)data;
    
    LOG_DEBUG("bucketIteratorItem: getting current item (worker thread)");
    
    UplinkBucketIterator* iterator = (UplinkBucketIterator*)work_data->iterator_handle;
    work_data->bucket = uplink_bucket_iterator_item(iterator);
}

/* ========== bucketIteratorErr execute ========== */

void bucket_iterator_err_execute(napi_env env, void* data) {
    (void)env;
    BucketIteratorErrData* work_data = (BucketIteratorErrData*)data;
    
    LOG_DEBUG("bucketIteratorErr: checking for error (worker thread)");
    
    UplinkBucketIterator* iterator = (UplinkBucketIterator*)work_data->iterator_handle;
    work_data->error = uplink_bucket_iterator_err(iterator);
}

/* ========== freeBucketIterator execute ========== */

void free_bucket_iterator_execute(napi_env env, void* data) {
    (void)env;
    FreeBucketIteratorData* work_data = (FreeBucketIteratorData*)data;
    
    LOG_DEBUG("freeBucketIterator: freeing iterator (worker thread)");
    
    UplinkBucketIterator* iterator = (UplinkBucketIterator*)work_data->iterator_handle;
    uplink_free_bucket_iterator(iterator);
}
