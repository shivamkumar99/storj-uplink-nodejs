/**
 * @file project_ops.h
 * @brief Project operations for uplink-nodejs
 * 
 * Provides N-API bindings for uplink-c project operations.
 */

#ifndef UPLINK_PROJECT_OPS_H
#define UPLINK_PROJECT_OPS_H

#include <node_api.h>

/**
 * Open project from access
 * JS: openProject(access: AccessHandle) -> Promise<ProjectHandle>
 */
napi_value open_project(napi_env env, napi_callback_info info);

/**
 * Open project with config
 * JS: configOpenProject(config, access) -> Promise<ProjectHandle>
 */
napi_value config_open_project(napi_env env, napi_callback_info info);

/**
 * Close project
 * JS: closeProject(project: ProjectHandle) -> Promise<void>
 */
napi_value close_project(napi_env env, napi_callback_info info);

/**
 * Revoke access grant
 * JS: revokeAccess(project: ProjectHandle, access: AccessHandle) -> Promise<void>
 *
 * Revokes the API key embedded in the provided access grant.
 * This is useful when you want to invalidate a previously shared access.
 */
napi_value revoke_access(napi_env env, napi_callback_info info);

#endif /* UPLINK_PROJECT_OPS_H */
