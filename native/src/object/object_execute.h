/**
 * @file object_execute.h
 * @brief Execute function declarations for object operations
 */

#ifndef OBJECT_EXECUTE_H
#define OBJECT_EXECUTE_H

#include <node_api.h>

/**
 * @brief Execute stat_object on worker thread
 */
void stat_object_execute(napi_env env, void* data);

/**
 * @brief Execute delete_object on worker thread
 */
void delete_object_execute(napi_env env, void* data);

/**
 * @brief Execute list_objects_create on worker thread
 */
void list_objects_create_execute(napi_env env, void* data);

/**
 * @brief Execute object_iterator_next on worker thread
 */
void object_iterator_next_execute(napi_env env, void* data);

/**
 * @brief Execute object_iterator_item on worker thread
 */
void object_iterator_item_execute(napi_env env, void* data);

/**
 * @brief Execute object_iterator_err on worker thread
 */
void object_iterator_err_execute(napi_env env, void* data);

/**
 * @brief Execute free_object_iterator on worker thread
 */
void free_object_iterator_execute(napi_env env, void* data);

/**
 * @brief Execute copy_object on worker thread
 */
void copy_object_execute(napi_env env, void* data);

/**
 * @brief Execute move_object on worker thread
 */
void move_object_execute(napi_env env, void* data);

/**
 * @brief Execute update_object_metadata on worker thread
 */
void update_object_metadata_execute(napi_env env, void* data);

#endif /* OBJECT_EXECUTE_H */
