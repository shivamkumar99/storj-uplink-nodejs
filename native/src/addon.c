/**
 * @file addon.c
 * @brief Native module entry point for uplink-nodejs
 * 
 * Initializes the Node.js native addon and exports functions.
 */

#include <node_api.h>
#include <stdio.h>
#include "common/logger.h"
#include "common/library_loader.h"
#include "common/error_registry.h"

/* Include operation modules */
#include "access/access_ops.h"
#include "project/project_ops.h"
#include "bucket/bucket_ops.h"
#include "object/object_ops.h"
#include "upload/upload_ops.h"
#include "download/download_ops.h"
#include "encryption/encryption_ops.h"
#include "multipart/multipart_ops.h"
#include "edge/edge_ops.h"
#include "debug/debug_ops.h"

/**
 * Helper macro to register a function
 */
#define DECLARE_NAPI_METHOD(name, func) \
    { name, 0, func, 0, 0, 0, napi_default, 0 }

/**
 * Module initialization function.
 * Called when the native module is loaded.
 */
static napi_value Init(napi_env env, napi_value exports) {
    /* Initialize logger */
    logger_init();
    LOG_INFO("Initializing uplink-nodejs native module");
    
    /* Load the uplink-c library */
    if (load_uplink_library() != 0) {
        LOG_WARN("uplink library not found - module will work in stub mode");
        /* Don't throw error here - allow module to load for testing */
        /* The actual functions will throw if library is not loaded */
    }
    
    /* Register access operations */
    napi_property_descriptor access_methods[] = {
        DECLARE_NAPI_METHOD("parseAccess", parse_access),
        DECLARE_NAPI_METHOD("requestAccessWithPassphrase", request_access_with_passphrase),
        DECLARE_NAPI_METHOD("configRequestAccessWithPassphrase", config_request_access_with_passphrase),
        DECLARE_NAPI_METHOD("accessSatelliteAddress", access_satellite_address),
        DECLARE_NAPI_METHOD("accessSerialize", access_serialize),
        DECLARE_NAPI_METHOD("accessShare", access_share),
        DECLARE_NAPI_METHOD("accessOverrideEncryptionKey", access_override_encryption_key),
    };
    
    napi_define_properties(env, exports, 
        sizeof(access_methods) / sizeof(access_methods[0]), 
        access_methods);
    
    /* Register project operations */
    napi_property_descriptor project_methods[] = {
        DECLARE_NAPI_METHOD("openProject", open_project),
        DECLARE_NAPI_METHOD("configOpenProject", config_open_project),
        DECLARE_NAPI_METHOD("closeProject", close_project),
        DECLARE_NAPI_METHOD("revokeAccess", revoke_access),
    };
    
    napi_define_properties(env, exports,
        sizeof(project_methods) / sizeof(project_methods[0]),
        project_methods);
    
    /* Register bucket operations */
    napi_property_descriptor bucket_methods[] = {
        DECLARE_NAPI_METHOD("createBucket", create_bucket),
        DECLARE_NAPI_METHOD("ensureBucket", ensure_bucket),
        DECLARE_NAPI_METHOD("statBucket", stat_bucket),
        DECLARE_NAPI_METHOD("deleteBucket", delete_bucket),
        DECLARE_NAPI_METHOD("deleteBucketWithObjects", delete_bucket_with_objects),
        DECLARE_NAPI_METHOD("listBucketsCreate", list_buckets_create),
        DECLARE_NAPI_METHOD("bucketIteratorNext", bucket_iterator_next),
        DECLARE_NAPI_METHOD("bucketIteratorItem", bucket_iterator_item),
        DECLARE_NAPI_METHOD("bucketIteratorErr", bucket_iterator_err),
        DECLARE_NAPI_METHOD("freeBucketIterator", free_bucket_iterator),
    };
    
    napi_define_properties(env, exports,
        sizeof(bucket_methods) / sizeof(bucket_methods[0]),
        bucket_methods);
    
    /* Register object operations */
    napi_property_descriptor object_methods[] = {
        DECLARE_NAPI_METHOD("statObject", stat_object),
        DECLARE_NAPI_METHOD("deleteObject", delete_object),
        DECLARE_NAPI_METHOD("listObjectsCreate", list_objects_create),
        DECLARE_NAPI_METHOD("objectIteratorNext", object_iterator_next),
        DECLARE_NAPI_METHOD("objectIteratorItem", object_iterator_item),
        DECLARE_NAPI_METHOD("objectIteratorErr", object_iterator_err),
        DECLARE_NAPI_METHOD("freeObjectIterator", free_object_iterator),
        DECLARE_NAPI_METHOD("copyObject", copy_object),
        DECLARE_NAPI_METHOD("moveObject", move_object),
        DECLARE_NAPI_METHOD("updateObjectMetadata", update_object_metadata),
    };
    
    napi_define_properties(env, exports,
        sizeof(object_methods) / sizeof(object_methods[0]),
        object_methods);
    
    /* Register upload operations */
    napi_property_descriptor upload_methods[] = {
        DECLARE_NAPI_METHOD("uploadObject", upload_object),
        DECLARE_NAPI_METHOD("uploadWrite", upload_write),
        DECLARE_NAPI_METHOD("uploadCommit", upload_commit),
        DECLARE_NAPI_METHOD("uploadAbort", upload_abort),
        DECLARE_NAPI_METHOD("uploadSetCustomMetadata", upload_set_custom_metadata),
        DECLARE_NAPI_METHOD("uploadInfo", upload_info),
    };
    
    napi_define_properties(env, exports,
        sizeof(upload_methods) / sizeof(upload_methods[0]),
        upload_methods);
    
    /* Register download operations */
    napi_property_descriptor download_methods[] = {
        DECLARE_NAPI_METHOD("downloadObject", download_object),
        DECLARE_NAPI_METHOD("downloadRead", download_read),
        DECLARE_NAPI_METHOD("downloadInfo", download_info),
        DECLARE_NAPI_METHOD("closeDownload", close_download),
    };
    
    napi_define_properties(env, exports,
        sizeof(download_methods) / sizeof(download_methods[0]),
        download_methods);
    
    /* Register encryption operations */
    napi_property_descriptor encryption_methods[] = {
        DECLARE_NAPI_METHOD("deriveEncryptionKey", derive_encryption_key),
    };
    
    napi_define_properties(env, exports,
        sizeof(encryption_methods) / sizeof(encryption_methods[0]),
        encryption_methods);
    
    /* Register multipart upload operations */
    napi_property_descriptor multipart_methods[] = {
        DECLARE_NAPI_METHOD("beginUpload", begin_upload),
        DECLARE_NAPI_METHOD("commitUpload", commit_upload),
        DECLARE_NAPI_METHOD("abortUpload", abort_upload),
        DECLARE_NAPI_METHOD("uploadPart", upload_part),
        DECLARE_NAPI_METHOD("partUploadWrite", part_upload_write),
        DECLARE_NAPI_METHOD("partUploadCommit", part_upload_commit),
        DECLARE_NAPI_METHOD("partUploadAbort", part_upload_abort),
        DECLARE_NAPI_METHOD("partUploadSetEtag", part_upload_set_etag),
        DECLARE_NAPI_METHOD("partUploadInfo", part_upload_info),
        DECLARE_NAPI_METHOD("listUploadPartsCreate", list_upload_parts_create),
        DECLARE_NAPI_METHOD("partIteratorNext", part_iterator_next),
        DECLARE_NAPI_METHOD("partIteratorItem", part_iterator_item),
        DECLARE_NAPI_METHOD("partIteratorErr", part_iterator_err),
        DECLARE_NAPI_METHOD("freePartIterator", free_part_iterator),
        DECLARE_NAPI_METHOD("listUploadsCreate", list_uploads_create),
        DECLARE_NAPI_METHOD("uploadIteratorNext", upload_iterator_next),
        DECLARE_NAPI_METHOD("uploadIteratorItem", upload_iterator_item),
        DECLARE_NAPI_METHOD("uploadIteratorErr", upload_iterator_err),
        DECLARE_NAPI_METHOD("freeUploadIterator", free_upload_iterator),
    };
    
    napi_define_properties(env, exports,
        sizeof(multipart_methods) / sizeof(multipart_methods[0]),
        multipart_methods);
    
    /* Register edge operations */
    napi_property_descriptor edge_methods[] = {
        DECLARE_NAPI_METHOD("edgeRegisterAccess", napi_edge_register_access),
        DECLARE_NAPI_METHOD("edgeJoinShareUrl", napi_edge_join_share_url),
    };

    napi_define_properties(env, exports,
        sizeof(edge_methods) / sizeof(edge_methods[0]),
        edge_methods);

    /* Register debug operations */
    napi_property_descriptor debug_methods[] = {
        DECLARE_NAPI_METHOD("internalUniverseIsEmpty", internal_universe_is_empty),
        DECLARE_NAPI_METHOD("testThrowTypedError", test_throw_typed_error),
    };

    napi_define_properties(env, exports,
        sizeof(debug_methods) / sizeof(debug_methods[0]),
        debug_methods);

    /* Register error registry operations */
    napi_property_descriptor error_methods[] = {
        DECLARE_NAPI_METHOD("initErrorClasses", napi_init_error_classes),
    };

    napi_define_properties(env, exports,
        sizeof(error_methods) / sizeof(error_methods[0]),
        error_methods);

    LOG_INFO("uplink-nodejs native module initialized successfully");
    LOG_INFO("Registered %d access, %d project, %d bucket, %d object, %d upload, %d download, %d encryption, %d multipart, %d edge, %d debug, %d error methods",
        (int)(sizeof(access_methods) / sizeof(access_methods[0])),
        (int)(sizeof(project_methods) / sizeof(project_methods[0])),
        (int)(sizeof(bucket_methods) / sizeof(bucket_methods[0])),
        (int)(sizeof(object_methods) / sizeof(object_methods[0])),
        (int)(sizeof(upload_methods) / sizeof(upload_methods[0])),
        (int)(sizeof(download_methods) / sizeof(download_methods[0])),
        (int)(sizeof(encryption_methods) / sizeof(encryption_methods[0])),
        (int)(sizeof(multipart_methods) / sizeof(multipart_methods[0])),
        (int)(sizeof(edge_methods) / sizeof(edge_methods[0])),
        (int)(sizeof(debug_methods) / sizeof(debug_methods[0])),
        (int)(sizeof(error_methods) / sizeof(error_methods[0])));
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)