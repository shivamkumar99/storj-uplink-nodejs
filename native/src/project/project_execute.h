/**
 * @file project_execute.h
 * @brief Execute function declarations for project async operations
 */

#ifndef PROJECT_EXECUTE_H
#define PROJECT_EXECUTE_H

#include <node_api.h>

/* Execute functions - run on worker thread */
void open_project_execute(napi_env env, void* data);
void config_open_project_execute(napi_env env, void* data);
void close_project_execute(napi_env env, void* data);
void revoke_access_execute(napi_env env, void* data);

#endif /* PROJECT_EXECUTE_H */
