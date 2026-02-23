/**
 * @file debug_ops.c
 * @brief Debug operation implementations for uplink-nodejs
 *
 * These functions provide debug utilities for testing and development.
 */

#include "debug_ops.h"
#include "../common/logger.h"
#include "../common/error_registry.h"
#include "../common/string_helpers.h"

/* Disable compat macros to avoid function name conflicts */
#define UPLINK_DISABLE_NAMESPACE_COMPAT
#include "uplink.h"

#include <stdlib.h>
#include <string.h>

/* ========== Async Work Data Structure ========== */

typedef struct {
    GoUint8 result;
    napi_deferred deferred;
    napi_async_work work;
} UniverseEmptyData;

/* ========== Execute function ========== */

static void universe_empty_execute(napi_env env, void* data) {
    (void)env;
    UniverseEmptyData* work_data = (UniverseEmptyData*)data;

    LOG_DEBUG("internalUniverseIsEmpty: checking (worker thread)");

    work_data->result = uplink_internal_UniverseIsEmpty();
}

/* ========== Complete function ========== */

static void universe_empty_complete(napi_env env, napi_status status, void* data) {
    UniverseEmptyData* work_data = (UniverseEmptyData*)data;

    if (status == napi_cancelled) {
        LOG_WARN("internalUniverseIsEmpty: operation cancelled");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Operation cancelled", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }

    napi_value result;
    napi_get_boolean(env, work_data->result != 0, &result);

    LOG_DEBUG("internalUniverseIsEmpty: %s", work_data->result ? "true" : "false");
    napi_resolve_deferred(env, work_data->deferred, result);

cleanup:
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

/* ========== Entry point function ========== */

napi_value internal_universe_is_empty(napi_env env, napi_callback_info info) {
    (void)info;

    UniverseEmptyData* work_data = (UniverseEmptyData*)calloc(1, sizeof(UniverseEmptyData));
    if (work_data == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }

    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);

    napi_value work_name;
    napi_create_string_utf8(env, "internalUniverseIsEmpty", NAPI_AUTO_LENGTH, &work_name);

    napi_create_async_work(
        env, NULL, work_name,
        universe_empty_execute,
        universe_empty_complete,
        work_data,
        &work_data->work
    );

    napi_queue_async_work(env, work_data->work);

    LOG_DEBUG("internalUniverseIsEmpty: queued async work");
    return promise;
}

/* ========== testThrowTypedError ========== */

/**
 * Async work data for test_throw_typed_error.
 * This is intentionally simple: the "execute" step does nothing,
 * and the "complete" step always rejects with a typed error.
 */
typedef struct {
    int32_t error_code;
    char* error_message;
    napi_deferred deferred;
    napi_async_work work;
} TestThrowData;

static void test_throw_execute(napi_env env, void* data) {
    (void)env;
    (void)data;
    /* Nothing to do on worker thread - error is created on main thread */
}

static void test_throw_complete(napi_env env, napi_status status, void* data) {
    TestThrowData* work_data = (TestThrowData*)data;

    if (status == napi_cancelled) {
        LOG_WARN("testThrowTypedError: operation cancelled");
        napi_value error;
        napi_value msg;
        napi_create_string_utf8(env, "Operation cancelled", NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, NULL, msg, &error);
        napi_reject_deferred(env, work_data->deferred, error);
        goto cleanup;
    }

    /* Create a typed error using the error registry */
    napi_value typed_error = create_typed_error(
        env, work_data->error_code, work_data->error_message);

    LOG_INFO("testThrowTypedError: rejecting with code=0x%02x, message=%s",
             work_data->error_code,
             work_data->error_message ? work_data->error_message : "(null)");

    napi_reject_deferred(env, work_data->deferred, typed_error);

cleanup:
    free(work_data->error_message);
    napi_delete_async_work(env, work_data->work);
    free(work_data);
}

napi_value test_throw_typed_error(napi_env env, napi_callback_info info) {
    /* Extract arguments: (code: number, message: string) */
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 2) {
        napi_throw_type_error(env, NULL,
            "testThrowTypedError requires 2 arguments: code (number), message (string)");
        return NULL;
    }

    /* Get error code */
    int32_t error_code;
    napi_get_value_int32(env, argv[0], &error_code);

    /* Get error message */
    size_t msg_len;
    napi_get_value_string_utf8(env, argv[1], NULL, 0, &msg_len);
    char* error_message = (char*)malloc(msg_len + 1);
    if (error_message == NULL) {
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }
    napi_get_value_string_utf8(env, argv[1], error_message, msg_len + 1, &msg_len);

    LOG_DEBUG("testThrowTypedError: code=0x%02x, message=%s", error_code, error_message);

    /* Allocate work data */
    TestThrowData* work_data = (TestThrowData*)calloc(1, sizeof(TestThrowData));
    if (work_data == NULL) {
        free(error_message);
        napi_throw_error(env, NULL, "Out of memory");
        return NULL;
    }

    work_data->error_code = error_code;
    work_data->error_message = error_message;

    /* Create promise */
    napi_value promise;
    napi_create_promise(env, &work_data->deferred, &promise);

    /* Queue async work */
    napi_value work_name;
    napi_create_string_utf8(env, "testThrowTypedError", NAPI_AUTO_LENGTH, &work_name);

    napi_create_async_work(
        env, NULL, work_name,
        test_throw_execute,
        test_throw_complete,
        work_data,
        &work_data->work
    );

    napi_queue_async_work(env, work_data->work);

    return promise;
}
