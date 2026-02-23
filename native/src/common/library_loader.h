/**
 * @file library_loader.h
 * @brief Dynamic library loader for uplink-c
 * 
 * Handles loading the uplink-c shared library at runtime.
 * Supports multiple platforms (Windows, macOS, Linux).
 */

#ifndef UPLINK_LIBRARY_LOADER_H
#define UPLINK_LIBRARY_LOADER_H

#include <stddef.h>

/**
 * Global library handle.
 * Set after successful library load.
 */
extern void* uplink_lib_handle;

/**
 * Load the uplink-c shared library.
 * Searches in multiple locations:
 * 1. UPLINK_LIBRARY_PATH environment variable
 * 2. native/prebuilds/<platform>/
 * 3. ./prebuilds/<platform>/
 * 4. System library path
 * 
 * @return 0 on success, -1 on failure
 */
int load_uplink_library(void);

/**
 * Unload the uplink-c shared library.
 */
void unload_uplink_library(void);

/**
 * Get a function pointer from the loaded library.
 * 
 * @param name Function name to look up
 * @return Function pointer, or NULL if not found
 */
void* get_uplink_function(const char* name);

/**
 * Check if the library is currently loaded.
 * 
 * @return 1 if loaded, 0 if not
 */
int is_library_loaded(void);

/**
 * Get the path of the loaded library.
 * 
 * @return Path string, or empty string if not loaded
 */
const char* get_loaded_library_path(void);

#endif /* UPLINK_LIBRARY_LOADER_H */
