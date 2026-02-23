/**
 * @file object_converter.h
 * @brief Shared UplinkObject to JavaScript conversion
 * 
 * Single implementation of UplinkObject* → JS object conversion,
 * used by object_complete.c, upload_complete.c, download_complete.c,
 * and multipart_complete.c.
 */

#ifndef UPLINK_OBJECT_CONVERTER_H
#define UPLINK_OBJECT_CONVERTER_H

#include <node_api.h>
#include "uplink.h"

/**
 * Convert an UplinkObject to a JavaScript object.
 * 
 * Produces a JS object with:
 *   { key: string, isPrefix: boolean,
 *     system: { created: Date, expires: Date|null, contentLength: number },
 *     custom: { [key: string]: string } }
 * 
 * - `system.created` is a JS Date (Unix seconds * 1000 → ms)
 * - `system.expires` is a JS Date if non-zero, or null if zero
 * - Returns undefined if object is NULL
 * 
 * @param env    N-API environment
 * @param object Pointer to UplinkObject (may be NULL)
 * @return napi_value representing the JS object
 */
napi_value uplink_object_to_js(napi_env env, UplinkObject* object);

/**
 * Free an array of UplinkCustomMetadataEntry up to @p count entries.
 * Frees each entry's key and value strings, then the array itself.
 *
 * @param entries  Pointer to the entry array (may be NULL)
 * @param count    Number of entries to free
 */
void free_metadata_entries(UplinkCustomMetadataEntry* entries, size_t count);

/**
 * Extract an array of UplinkCustomMetadataEntry from a JS object whose
 * own-property values must all be strings.
 *
 * @param env           N-API environment
 * @param js_meta       JS object with string key-value pairs
 * @param[out] out_entries  Receives the malloc'd entry array (NULL when count is 0)
 * @param[out] out_count    Receives the number of entries
 * @return  0 on success, -1 if a value is not a string, -2 on OOM
 */
int extract_metadata_entries_from_js(napi_env env, napi_value js_meta,
                                     UplinkCustomMetadataEntry** out_entries,
                                     size_t* out_count);

#endif /* UPLINK_OBJECT_CONVERTER_H */
