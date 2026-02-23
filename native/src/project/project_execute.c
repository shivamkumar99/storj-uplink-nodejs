/**
 * @file project_execute.c
 * @brief Execute function implementations for project async operations
 * 
 * These functions run on worker threads and call uplink-c functions.
 */

#include "project_execute.h"
#include "project_types.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== open_project execute ========== */

void open_project_execute(napi_env env, void* data) {
    (void)env;
    OpenProjectData* work_data = (OpenProjectData*)data;
    
    LOG_DEBUG("openProject: opening project (worker thread)");
    
    UplinkAccess access = { ._handle = work_data->access_handle };
    work_data->result = uplink_open_project(&access);
}

/* ========== config_open_project execute ========== */

void config_open_project_execute(napi_env env, void* data) {
    (void)env;
    ConfigOpenProjectData* work_data = (ConfigOpenProjectData*)data;
    
    LOG_DEBUG("configOpenProject: opening project with config (worker thread)");
    
    UplinkConfig config = {
        .user_agent = work_data->user_agent,
        .dial_timeout_milliseconds = work_data->dial_timeout_milliseconds,
        .temp_directory = work_data->temp_directory
    };
    
    UplinkAccess access = { ._handle = work_data->access_handle };
    work_data->result = uplink_config_open_project(config, &access);
}

/* ========== close_project execute ========== */

void close_project_execute(napi_env env, void* data) {
    (void)env;
    CloseProjectData* work_data = (CloseProjectData*)data;

    LOG_DEBUG("closeProject: closing project (worker thread)");

    UplinkProject project = { ._handle = work_data->project_handle };
    work_data->error = uplink_close_project(&project);
}

/* ========== revoke_access execute ========== */

void revoke_access_execute(napi_env env, void* data) {
    (void)env;
    RevokeAccessData* work_data = (RevokeAccessData*)data;

    LOG_DEBUG("revokeAccess: revoking access (worker thread)");

    UplinkProject project = { ._handle = work_data->project_handle };
    UplinkAccess access = { ._handle = work_data->access_handle };
    work_data->error = uplink_revoke_access(&project, &access);
}
