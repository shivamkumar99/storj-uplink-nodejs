/**
 * @file edge_complete.h
 * @brief Complete function declarations for edge async operations
 * 
 * These functions run on the main thread after execute completes.
 * They handle result conversion to JavaScript and cleanup.
 */

#ifndef UPLINK_EDGE_COMPLETE_H
#define UPLINK_EDGE_COMPLETE_H

#include <node_api.h>

/**
 * @brief Complete edgeRegisterAccess on main thread
 */
void register_access_complete(napi_env env, napi_status status, void* data);

/**
 * @brief Complete edgeJoinShareUrl on main thread
 */
void join_share_url_complete(napi_env env, napi_status status, void* data);

#endif /* UPLINK_EDGE_COMPLETE_H */
