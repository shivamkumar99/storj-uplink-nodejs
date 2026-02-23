/**
 * @file object_execute.c
 * @brief Worker thread execute functions for object operations
 */

#include "object_execute.h"
#include "object_types.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== stat_object_execute ========== */

void stat_object_execute(napi_env env, void* data) {
    (void)env;
    ObjectOpData* work_data = (ObjectOpData*)data;
    
    LOG_DEBUG("statObject: getting info for '%s/%s' (worker thread)", 
              work_data->bucket_name, work_data->object_key);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_stat_object(&project, work_data->bucket_name, work_data->object_key);
}

/* ========== delete_object_execute ========== */

void delete_object_execute(napi_env env, void* data) {
    (void)env;
    ObjectOpData* work_data = (ObjectOpData*)data;
    
    LOG_DEBUG("deleteObject: deleting '%s/%s' (worker thread)", 
              work_data->bucket_name, work_data->object_key);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_delete_object(&project, work_data->bucket_name, work_data->object_key);
}

/* ========== listObjectsCreate execute ========== */

void list_objects_create_execute(napi_env env, void* data) {
    (void)env;
    ListObjectsCreateData* work_data = (ListObjectsCreateData*)data;
    
    LOG_DEBUG("listObjectsCreate: creating object iterator for bucket '%s' (worker thread)", 
              work_data->bucket_name);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    /* Prepare options */
    UplinkListObjectsOptions options = { 0 };
    options.prefix = work_data->prefix;
    options.cursor = work_data->cursor;
    options.recursive = work_data->recursive;
    options.system = work_data->include_system;
    options.custom = work_data->include_custom;
    
    UplinkObjectIterator* iterator = uplink_list_objects(&project, work_data->bucket_name, &options);
    work_data->iterator_handle = (size_t)iterator;
    
    LOG_DEBUG("listObjectsCreate: iterator created, handle=%zu", work_data->iterator_handle);
}

/* ========== objectIteratorNext execute ========== */

void object_iterator_next_execute(napi_env env, void* data) {
    (void)env;
    ObjectIteratorNextData* work_data = (ObjectIteratorNextData*)data;
    
    LOG_DEBUG("objectIteratorNext: advancing iterator (worker thread)");
    
    UplinkObjectIterator* iterator = (UplinkObjectIterator*)work_data->iterator_handle;
    work_data->has_next = uplink_object_iterator_next(iterator);
    
    LOG_DEBUG("objectIteratorNext: has_next=%d", work_data->has_next);
}

/* ========== objectIteratorItem execute ========== */

void object_iterator_item_execute(napi_env env, void* data) {
    (void)env;
    ObjectIteratorItemData* work_data = (ObjectIteratorItemData*)data;
    
    LOG_DEBUG("objectIteratorItem: getting current item (worker thread)");
    
    UplinkObjectIterator* iterator = (UplinkObjectIterator*)work_data->iterator_handle;
    work_data->object = uplink_object_iterator_item(iterator);
}

/* ========== objectIteratorErr execute ========== */

void object_iterator_err_execute(napi_env env, void* data) {
    (void)env;
    ObjectIteratorErrData* work_data = (ObjectIteratorErrData*)data;
    
    LOG_DEBUG("objectIteratorErr: checking for error (worker thread)");
    
    UplinkObjectIterator* iterator = (UplinkObjectIterator*)work_data->iterator_handle;
    work_data->error = uplink_object_iterator_err(iterator);
}

/* ========== freeObjectIterator execute ========== */

void free_object_iterator_execute(napi_env env, void* data) {
    (void)env;
    FreeObjectIteratorData* work_data = (FreeObjectIteratorData*)data;
    
    LOG_DEBUG("freeObjectIterator: freeing iterator (worker thread)");
    
    UplinkObjectIterator* iterator = (UplinkObjectIterator*)work_data->iterator_handle;
    uplink_free_object_iterator(iterator);
}

/* ========== copy_object_execute ========== */

void copy_object_execute(napi_env env, void* data) {
    (void)env;
    CopyMoveObjectData* work_data = (CopyMoveObjectData*)data;
    
    LOG_DEBUG("copyObject: copying '%s/%s' -> '%s/%s' (worker thread)", 
              work_data->src_bucket, work_data->src_key,
              work_data->dst_bucket, work_data->dst_key);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->result = uplink_copy_object(
        &project,
        work_data->src_bucket,
        work_data->src_key,
        work_data->dst_bucket,
        work_data->dst_key,
        NULL  /* options */
    );
}

/* ========== move_object_execute ========== */

void move_object_execute(napi_env env, void* data) {
    (void)env;
    CopyMoveObjectData* work_data = (CopyMoveObjectData*)data;
    
    LOG_DEBUG("moveObject: moving '%s/%s' -> '%s/%s' (worker thread)", 
              work_data->src_bucket, work_data->src_key,
              work_data->dst_bucket, work_data->dst_key);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    /* uplink_move_object returns UplinkError* directly */
    work_data->move_error = uplink_move_object(
        &project,
        work_data->src_bucket,
        work_data->src_key,
        work_data->dst_bucket,
        work_data->dst_key,
        NULL  /* options */
    );
}

/* ========== update_object_metadata_execute ========== */

void update_object_metadata_execute(napi_env env, void* data) {
    (void)env;
    UpdateMetadataData* work_data = (UpdateMetadataData*)data;
    
    LOG_DEBUG("updateObjectMetadata: updating metadata for '%s/%s' (worker thread)", 
              work_data->bucket_name, work_data->object_key);
    
    UplinkProject project = { ._handle = work_data->project_handle };
    
    UplinkCustomMetadata metadata = {0};
    metadata.entries = work_data->metadata_entries;
    metadata.count = work_data->metadata_count;
    
    work_data->error = uplink_update_object_metadata(&project, work_data->bucket_name, 
                                                      work_data->object_key, metadata, NULL);
}
