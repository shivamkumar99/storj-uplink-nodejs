/**
 * @file project_complete.h
 * @brief Complete function declarations for project async operations
 */

#ifndef PROJECT_COMPLETE_H
#define PROJECT_COMPLETE_H

#include <node_api.h>

/* Complete functions - run on main thread */
void open_project_complete(napi_env env, napi_status status, void* data);
void config_open_project_complete(napi_env env, napi_status status, void* data);
void close_project_complete(napi_env env, napi_status status, void* data);
void revoke_access_complete(napi_env env, napi_status status, void* data);

#endif /* PROJECT_COMPLETE_H */
