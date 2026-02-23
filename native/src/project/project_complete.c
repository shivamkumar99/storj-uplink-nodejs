/**
 * @file project_complete.c
 * @brief Complete function implementations for project async operations
 * 
 * These functions run on the main thread and handle promise resolution/rejection.
 */

#include "project_complete.h"
#include "project_types.h"
#include "../common/handle_helpers.h"
#include "../common/result_helpers.h"
#include "../common/error_registry.h"
#include "../common/cancel_helpers.h"
#include "../common/logger.h"

#include <stdlib.h>

/* ========== open_project complete ========== */

void open_project_complete(napi_env env, napi_status status, void* data) {
    OpenProjectData* work_data = (OpenProjectData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "openProject");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("openProject: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value project_external = create_handle_external(
        env,
        work_data->result.project->_handle,
        HANDLE_TYPE_PROJECT,
        work_data->result.project,
        NULL
    );
    
    if (project_external == NULL) {
        LOG_ERROR("openProject: failed to create handle external");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create project handle", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    LOG_INFO("openProject: success, handle=%zu", work_data->result.project->_handle);
    napi_resolve_deferred(env, work_data->deferred, project_external);
    
cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== config_open_project complete ========== */

void config_open_project_complete(napi_env env, napi_status status, void* data) {
    ConfigOpenProjectData* work_data = (ConfigOpenProjectData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "configOpenProject");
    
    if (work_data->result.error != NULL) {
        LOG_ERROR("configOpenProject: failed - %s", work_data->result.error->message);
        napi_value error = create_typed_error(env, work_data->result.error->code, work_data->result.error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->result.error);
        goto cleanup;
    }
    
    napi_value project_external = create_handle_external(
        env,
        work_data->result.project->_handle,
        HANDLE_TYPE_PROJECT,
        work_data->result.project,
        NULL
    );
    
    if (project_external == NULL) {
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Failed to create project handle", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }
    
    LOG_INFO("configOpenProject: success, handle=%zu", work_data->result.project->_handle);
    napi_resolve_deferred(env, work_data->deferred, project_external);
    
cleanup:
    free(work_data->user_agent);
    free(work_data->temp_directory);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== close_project complete ========== */

void close_project_complete(napi_env env, napi_status status, void* data) {
    CloseProjectData* work_data = (CloseProjectData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "closeProject");

    if (work_data->error != NULL) {
        LOG_ERROR("closeProject: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }

    LOG_INFO("closeProject: success");
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);

cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== revoke_access complete ========== */

void revoke_access_complete(napi_env env, napi_status status, void* data) {
    RevokeAccessData* work_data = (RevokeAccessData*)data;
    REJECT_IF_CANCELLED(env, status, work_data->deferred, "revokeAccess");

    if (work_data->error != NULL) {
        LOG_ERROR("revokeAccess: failed - %s", work_data->error->message);
        napi_value error = create_typed_error(env, work_data->error->code, work_data->error->message);
        napi_reject_deferred(env, work_data->deferred, error);
        uplink_free_error(work_data->error);
        goto cleanup;
    }

    LOG_INFO("revokeAccess: success");
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    napi_resolve_deferred(env, work_data->deferred, undefined);

cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}
