/**
 * @file object_complete.c
 * @brief Main thread completion handlers for object operations
 */

#include "object_complete.h"
#include "object_types.h"
#include "../common/handle_helpers.h"
#include "../common/result_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/object_converter.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== stat_object_complete ========== */

void stat_object_complete(napi_env env, napi_status status, void* data) {
    ObjectOpData* work_data = (ObjectOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "statObject");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("statObject: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value object_obj = uplink_object_to_js(env, work_data->result.object);
    uplink_free_object_result(work_data->result);
    
    LOG_INFO("statObject: got info for '%s/%s'", work_data->bucket_name, work_data->object_key);
    napi_resolve_deferred(env, work_data->deferred, object_obj);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== delete_object_complete ========== */

void delete_object_complete(napi_env env, napi_status status, void* data) {
    ObjectOpData* work_data = (ObjectOpData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "deleteObject");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("deleteObject: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    /* Free the returned object if any */
    if (work_data->result.object != NULL) {
        uplink_free_object(work_data->result.object);
    }
    
    LOG_INFO("deleteObject: deleted '%s/%s'", work_data->bucket_name, work_data->object_key);
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== listObjectsCreate complete ========== */

void list_objects_create_complete(napi_env env, napi_status status, void* data) {
    ListObjectsCreateData* work_data = (ListObjectsCreateData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "listObjectsCreate");
    
    if (work_data->iterator_handle == 0) {
        LOG_ERROR("listObjectsCreate: failed to create iterator");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create object iterator", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    {
        napi_value handle_obj = create_handle_external(env, work_data->iterator_handle, 
                                                        HANDLE_TYPE_OBJECT_ITERATOR, NULL, NULL);
        
        LOG_INFO("listObjectsCreate: iterator created, handle=%zu", work_data->iterator_handle);
        napi_resolve_deferred(env, work_data->deferred, handle_obj);
    }
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->prefix);
    free(work_data->cursor);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== objectIteratorNext complete ========== */

void object_iterator_next_complete(napi_env env, napi_status status, void* data) {
    ObjectIteratorNextData* work_data = (ObjectIteratorNextData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "objectIteratorNext");
    
    {
        napi_value result;
        napi_get_boolean(env, work_data->has_next, &result);
        napi_resolve_deferred(env, work_data->deferred, result);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== objectIteratorItem complete ========== */

void object_iterator_item_complete(napi_env env, napi_status status, void* data) {
    ObjectIteratorItemData* work_data = (ObjectIteratorItemData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "objectIteratorItem");
    
    {
        napi_value object_obj = uplink_object_to_js(env, work_data->object);
        
        if (work_data->object != NULL) {
            uplink_free_object(work_data->object);
        }
        
        LOG_DEBUG("objectIteratorItem: returned object item");
        napi_resolve_deferred(env, work_data->deferred, object_obj);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== objectIteratorErr complete ========== */

void object_iterator_err_complete(napi_env env, napi_status status, void* data) {
    ObjectIteratorErrData* work_data = (ObjectIteratorErrData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "objectIteratorErr");
    
    if (work_data->error != NULL) {
        LOG_ERROR("objectIteratorErr: iteration error - %s", work_data->error->message);
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

/* ========== freeObjectIterator complete ========== */

void free_object_iterator_complete(napi_env env, napi_status status, void* data) {
    FreeObjectIteratorData* work_data = (FreeObjectIteratorData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "freeObjectIterator");
    
    LOG_INFO("freeObjectIterator: iterator freed");
    
    {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_resolve_deferred(env, work_data->deferred, undefined);
    }
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== copy_object_complete ========== */

void copy_object_complete(napi_env env, napi_status status, void* data) {
    CopyMoveObjectData* work_data = (CopyMoveObjectData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "copyObject");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("copyObject: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value object_obj = uplink_object_to_js(env, work_data->result.object);
    uplink_free_object_result(work_data->result);
    
    LOG_INFO("copyObject: copied '%s/%s' -> '%s/%s'", 
             work_data->src_bucket, work_data->src_key,
             work_data->dst_bucket, work_data->dst_key);
    napi_resolve_deferred(env, work_data->deferred, object_obj);
    
cleanup:
    free(work_data->src_bucket);
    free(work_data->src_key);
    free(work_data->dst_bucket);
    free(work_data->dst_key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== move_object_complete ========== */

void move_object_complete(napi_env env, napi_status status, void* data) {
    CopyMoveObjectData* work_data = (CopyMoveObjectData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "moveObject");
    
    if (work_data->move_error != NULL) {
        LOG_ERROR("moveObject: failed - %s", work_data->move_error->message);
        napi_value error = create_typed_error(env, work_data->move_error->code, work_data->move_error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->move_error);
        goto cleanup;
    }
    
    LOG_INFO("moveObject: moved '%s/%s' -> '%s/%s'", 
             work_data->src_bucket, work_data->src_key,
             work_data->dst_bucket, work_data->dst_key);
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    free(work_data->src_bucket);
    free(work_data->src_key);
    free(work_data->dst_bucket);
    free(work_data->dst_key);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== update_object_metadata_complete ========== */

void update_object_metadata_complete(napi_env env, napi_status status, void* data) {
    UpdateMetadataData* work_data = (UpdateMetadataData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "updateObjectMetadata");
    
    if (work_data->error != NULL) {
        LOG_ERROR("updateObjectMetadata: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }
    
    LOG_INFO("updateObjectMetadata: metadata updated for '%s/%s'", 
             work_data->bucket_name, work_data->object_key);
    
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);
    
cleanup:
    free(work_data->bucket_name);
    free(work_data->object_key);
    
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
