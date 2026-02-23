/**
 * @file object_complete.h
 * @brief Complete function declarations for object operations
 */

#ifndef OBJECT_COMPLETE_H
#define OBJECT_COMPLETE_H

#include <node_api.h>

/**
 * @brief Complete stat_object on main thread
 */
void stat_object_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete delete_object on main thread
 */
void delete_object_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete list_objects_create on main thread
 */
void list_objects_create_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete object_iterator_next on main thread
 */
void object_iterator_next_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete object_iterator_item on main thread
 */
void object_iterator_item_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete object_iterator_err on main thread
 */
void object_iterator_err_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete free_object_iterator on main thread
 */
void free_object_iterator_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete copy_object on main thread
 */
void copy_object_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete move_object on main thread
 */
void move_object_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete update_object_metadata on main thread
 */
void update_object_metadata_complete(napi_env env, napi_status status, void* data);

#endif /* OBJECT_COMPLETE_H */
