/**
 * @file edge_execute.h
 * @brief Execute function declarations for edge async operations
 * 
 * These functions run on the worker thread and call uplink-c library functions.
 */

#ifndef UPLINK_EDGE_EXECUTE_H
#define UPLINK_EDGE_EXECUTE_H

#include <node_api.h>

/**
 * @brief Execute edgeRegisterAccess on worker thread
 */
void register_access_execute(napi_env env, void* data);

/**
 * @brief Execute edgeJoinShareUrl on worker thread
 */
void join_share_url_execute(napi_env env, void* data);

#endif /* UPLINK_EDGE_EXECUTE_H */
