/**
 * @file project_ops.c
 * @brief N-API entry points for project operations
 * 
 * This file contains the N-API entry point functions that:
 * 1. Extract arguments from JavaScript
 * 2. Validate inputs
 * 3. Set up async work
 * 4. Return promises
 */

#include "project_ops.h"
#include "project_types.h"
#include "project_execute.h"
#include "project_complete.h"
#include "../common/handle_helpers.h"
#include "../common/string_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>
#include <string.h>

/* ========== open_project ========== */

napi_value open_project(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "access handle is required");
        return NULL;
    }
    
    size_t access_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_ACCESS, &access_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }
    
    OpenProjectData* work_data = (OpenProjectData*)calloc(1, sizeof(OpenProjectData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->access_handle = access_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "openProject", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        open_project_execute,
        open_project_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    LOG_DEBUG("openProject: queued async work");
    return promise;
}

/* ========== config_open_project ========== */

napi_value config_open_project(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "config and access are required");
        return NULL;
    }
    
    /* Extract config object */
    napi_valuetype config_type;
    napi_typeof(env, argv[0], &config_type);
    if (config_type != napi_object) {
        napi_throw_type_error(env, NULL, "config must be an object");
        return NULL;
    }
    
    size_t access_handle;
    if (extract_handle(env, argv[1], HANDLE_TYPE_ACCESS, &access_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }
    
    ConfigOpenProjectData* work_data = (ConfigOpenProjectData*)calloc(1, sizeof(ConfigOpenProjectData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->access_handle = access_handle;
    
    /* Extract config properties */
    napi_value user_agent_val, dial_timeout_val, temp_dir_val;
    
    napi_get_named_property(env, argv[0], "userAgent", &user_agent_val);
    napi_get_named_property(env, argv[0], "dialTimeoutMilliseconds", &dial_timeout_val);
    napi_get_named_property(env, argv[0], "tempDirectory", &temp_dir_val);
    
    extract_string_optional(env, user_agent_val, &work_data->user_agent);
    extract_string_optional(env, temp_dir_val, &work_data->temp_directory);
    
    napi_valuetype timeout_type;
    napi_typeof(env, dial_timeout_val, &timeout_type);
    if (timeout_type == napi_number) {
        napi_get_value_int32(env, dial_timeout_val, &work_data->dial_timeout_milliseconds);
    }
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "configOpenProject", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        config_open_project_execute,
        config_open_project_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    return promise;
}

/* ========== close_project ========== */

napi_value close_project(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "project handle is required");
        return NULL;
    }
    
    size_t project_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }
    
    CloseProjectData* work_data = (CloseProjectData*)calloc(1, sizeof(CloseProjectData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    
    work_data->project_handle = project_handle;
    
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);
    
    napi_value work_name;
    napi_create_string_utf8(env, "closeProject", NAPI_AUTO_LENGTH, &work_name);
    
    napi_create_async_work(
        env, NULL, work_name,
        close_project_execute,
        close_project_complete,
        work_data,
        &work_data->work
    );
    
    napi_queue_async_work(env, work_data->work);
    
    LOG_DEBUG("closeProject: queued async work");
    return promise;
}

/* ========== revoke_access ========== */

napi_value revoke_access(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "project and access handles are required");
        return NULL;
    }

    size_t project_handle;
    if (extract_handle(env, argv[0], HANDLE_TYPE_PROJECT, &project_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid project handle");
        return NULL;
    }

    size_t access_handle;
    if (extract_handle(env, argv[1], HANDLE_TYPE_ACCESS, &access_handle) != napi_ok) {
        napi_throw_type_error(env, NULL, "Invalid access handle");
        return NULL;
    }

    RevokeAccessData* work_data = (RevokeAccessData*)calloc(1, sizeof(RevokeAccessData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }

    work_data->project_handle = project_handle;
    work_data->access_handle = access_handle;

    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);

    napi_value work_name;
    napi_create_string_utf8(env, "revokeAccess", NAPI_AUTO_LENGTH, &work_name);

    napi_create_async_work(
        env, NULL, work_name,
        revoke_access_execute,
        revoke_access_complete,
        work_data,
        &work_data->work
    );

    napi_queue_async_work(env, work_data->work);

    LOG_DEBUG("revokeAccess: queued async work");
    return promise;
}
