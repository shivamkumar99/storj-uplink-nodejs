/**
 * @file upload_execute.h
 * @brief Execute function declarations for upload async operations
 */

#ifndef UPLOAD_EXECUTE_H
#define UPLOAD_EXECUTE_H

#include <node_api.h>

/* Execute functions - run on worker thread */
void upload_object_execute(napi_env env, void* data);
void upload_write_execute(napi_env env, void* data);
void upload_commit_execute(napi_env env, void* data);
void upload_abort_execute(napi_env env, void* data);
void upload_set_metadata_execute(napi_env env, void* data);
void upload_info_execute(napi_env env, void* data);

#endif /* UPLOAD_EXECUTE_H */
