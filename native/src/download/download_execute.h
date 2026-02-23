/**
 * @file download_execute.h
 * @brief Execute function declarations for download async operations
 */

#ifndef DOWNLOAD_EXECUTE_H
#define DOWNLOAD_EXECUTE_H

#include <node_api.h>

/* Execute functions - run on worker thread */
void download_object_execute(napi_env env, void* data);
void download_read_execute(napi_env env, void* data);
void download_info_execute(napi_env env, void* data);
void close_download_execute(napi_env env, void* data);

#endif /* DOWNLOAD_EXECUTE_H */
