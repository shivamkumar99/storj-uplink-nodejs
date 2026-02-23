/**
 * @file native/test/test_handle_helpers.c
 * @brief Unit tests for handle helper functions
 */

#include "test_framework.h"
#include <stdlib.h>
#include <string.h>

/* Handle types enum (copied from handle_helpers.h for standalone testing) */
typedef enum {
    HANDLE_TYPE_ACCESS = 0,
    HANDLE_TYPE_PROJECT,
    HANDLE_TYPE_DOWNLOAD,
    HANDLE_TYPE_UPLOAD,
    HANDLE_TYPE_ENCRYPTION_KEY,
    HANDLE_TYPE_PART_UPLOAD,
    HANDLE_TYPE_OBJECT_ITERATOR,
    HANDLE_TYPE_BUCKET_ITERATOR,
    HANDLE_TYPE_UPLOAD_ITERATOR,
    HANDLE_TYPE_PART_ITERATOR,
    HANDLE_TYPE_COUNT
} HandleType;

/* Handle type names */
static const char* handle_type_names[] = {
    "Access",
    "Project",
    "Download",
    "Upload",
    "EncryptionKey",
    "PartUpload",
    "ObjectIterator",
    "BucketIterator",
    "UploadIterator",
    "PartIterator"
};

static const char* get_handle_type_name(HandleType type) {
    if (type >= 0 && type < HANDLE_TYPE_COUNT) {
        return handle_type_names[type];
    }
    return "Unknown";
}

/* Simple handle map simulation for testing */
typedef struct {
    size_t id;
    void* ptr;
    HandleType type;
    int active;
} HandleEntry;

#define MAX_HANDLES 100
static HandleEntry handle_map[MAX_HANDLES];
static size_t next_handle_id = 1;

static void init_handle_map(void) {
    memset(handle_map, 0, sizeof(handle_map));
    next_handle_id = 1;
}

static size_t store_handle(void* ptr, HandleType type) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (!handle_map[i].active) {
            handle_map[i].id = next_handle_id++;
            handle_map[i].ptr = ptr;
            handle_map[i].type = type;
            handle_map[i].active = 1;
            return handle_map[i].id;
        }
    }
    return 0; /* No space */
}

static void* get_handle(size_t id, HandleType type) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (handle_map[i].active && handle_map[i].id == id && handle_map[i].type == type) {
            return handle_map[i].ptr;
        }
    }
    return NULL;
}

static int remove_handle(size_t id, HandleType type) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (handle_map[i].active && handle_map[i].id == id && handle_map[i].type == type) {
            handle_map[i].active = 0;
            return 1;
        }
    }
    return 0;
}

static int validate_handle(size_t handle, HandleType type) {
    return get_handle(handle, type) != NULL;
}

/* ==================== Tests ==================== */

int test_handle_type_names(void) {
    TEST_ASSERT_STR_EQ(get_handle_type_name(HANDLE_TYPE_ACCESS), "Access", "Access type name");
    TEST_ASSERT_STR_EQ(get_handle_type_name(HANDLE_TYPE_PROJECT), "Project", "Project type name");
    TEST_ASSERT_STR_EQ(get_handle_type_name(HANDLE_TYPE_DOWNLOAD), "Download", "Download type name");
    TEST_ASSERT_STR_EQ(get_handle_type_name(HANDLE_TYPE_UPLOAD), "Upload", "Upload type name");
    TEST_ASSERT_STR_EQ(get_handle_type_name(HANDLE_TYPE_ENCRYPTION_KEY), "EncryptionKey", "EncryptionKey type name");
    TEST_ASSERT_STR_EQ(get_handle_type_name(HANDLE_TYPE_PART_UPLOAD), "PartUpload", "PartUpload type name");
    return 1;
}

int test_handle_type_unknown(void) {
    TEST_ASSERT_STR_EQ(get_handle_type_name((HandleType)999), "Unknown", "Unknown type");
    TEST_ASSERT_STR_EQ(get_handle_type_name((HandleType)-1), "Unknown", "Negative type");
    return 1;
}

int test_handle_store_and_retrieve(void) {
    init_handle_map();
    
    int dummy_data1 = 42;
    int dummy_data2 = 99;
    
    size_t handle1 = store_handle(&dummy_data1, HANDLE_TYPE_ACCESS);
    size_t handle2 = store_handle(&dummy_data2, HANDLE_TYPE_PROJECT);
    
    TEST_ASSERT(handle1 != 0, "Handle 1 should be non-zero");
    TEST_ASSERT(handle2 != 0, "Handle 2 should be non-zero");
    TEST_ASSERT(handle1 != handle2, "Handles should be unique");
    
    void* retrieved1 = get_handle(handle1, HANDLE_TYPE_ACCESS);
    void* retrieved2 = get_handle(handle2, HANDLE_TYPE_PROJECT);
    
    TEST_ASSERT(retrieved1 == &dummy_data1, "Retrieved handle 1 should match");
    TEST_ASSERT(retrieved2 == &dummy_data2, "Retrieved handle 2 should match");
    
    return 1;
}

int test_handle_wrong_type(void) {
    init_handle_map();
    
    int dummy_data = 42;
    size_t handle = store_handle(&dummy_data, HANDLE_TYPE_ACCESS);
    
    /* Try to retrieve with wrong type */
    void* retrieved = get_handle(handle, HANDLE_TYPE_PROJECT);
    TEST_ASSERT_NULL(retrieved, "Wrong type should return NULL");
    
    /* Correct type should work */
    retrieved = get_handle(handle, HANDLE_TYPE_ACCESS);
    TEST_ASSERT_NOT_NULL(retrieved, "Correct type should return pointer");
    
    return 1;
}

int test_handle_remove(void) {
    init_handle_map();
    
    int dummy_data = 42;
    size_t handle = store_handle(&dummy_data, HANDLE_TYPE_ACCESS);
    
    /* Verify handle exists */
    TEST_ASSERT_EQ(validate_handle(handle, HANDLE_TYPE_ACCESS), 1, "Handle should be valid");
    
    /* Remove handle */
    int result = remove_handle(handle, HANDLE_TYPE_ACCESS);
    TEST_ASSERT_EQ(result, 1, "Remove should succeed");
    
    /* Verify handle no longer exists */
    TEST_ASSERT_EQ(validate_handle(handle, HANDLE_TYPE_ACCESS), 0, "Handle should be invalid after removal");
    
    return 1;
}

int test_handle_remove_wrong_type(void) {
    init_handle_map();
    
    int dummy_data = 42;
    size_t handle = store_handle(&dummy_data, HANDLE_TYPE_ACCESS);
    
    /* Try to remove with wrong type */
    int result = remove_handle(handle, HANDLE_TYPE_PROJECT);
    TEST_ASSERT_EQ(result, 0, "Remove with wrong type should fail");
    
    /* Handle should still exist */
    TEST_ASSERT_EQ(validate_handle(handle, HANDLE_TYPE_ACCESS), 1, "Handle should still be valid");
    
    return 1;
}

int test_handle_invalid_id(void) {
    init_handle_map();
    
    /* Try to get non-existent handle */
    void* retrieved = get_handle(99999, HANDLE_TYPE_ACCESS);
    TEST_ASSERT_NULL(retrieved, "Invalid ID should return NULL");
    
    /* Try to validate non-existent handle */
    TEST_ASSERT_EQ(validate_handle(99999, HANDLE_TYPE_ACCESS), 0, "Invalid ID should not validate");
    
    return 1;
}

int test_handle_zero_id(void) {
    init_handle_map();
    
    /* Zero handle should not be valid */
    void* retrieved = get_handle(0, HANDLE_TYPE_ACCESS);
    TEST_ASSERT_NULL(retrieved, "Zero handle should return NULL");
    
    TEST_ASSERT_EQ(validate_handle(0, HANDLE_TYPE_ACCESS), 0, "Zero handle should not validate");
    
    return 1;
}

int test_handle_multiple_types(void) {
    init_handle_map();
    
    int data1 = 1, data2 = 2, data3 = 3;
    
    size_t access_handle = store_handle(&data1, HANDLE_TYPE_ACCESS);
    size_t project_handle = store_handle(&data2, HANDLE_TYPE_PROJECT);
    size_t upload_handle = store_handle(&data3, HANDLE_TYPE_UPLOAD);
    
    /* All handles should be unique */
    TEST_ASSERT(access_handle != project_handle, "Access and project handles should differ");
    TEST_ASSERT(project_handle != upload_handle, "Project and upload handles should differ");
    
    /* Each should only validate for correct type */
    TEST_ASSERT_EQ(validate_handle(access_handle, HANDLE_TYPE_ACCESS), 1, "Access handle valid for access type");
    TEST_ASSERT_EQ(validate_handle(access_handle, HANDLE_TYPE_PROJECT), 0, "Access handle invalid for project type");
    
    TEST_ASSERT_EQ(validate_handle(project_handle, HANDLE_TYPE_PROJECT), 1, "Project handle valid for project type");
    TEST_ASSERT_EQ(validate_handle(project_handle, HANDLE_TYPE_UPLOAD), 0, "Project handle invalid for upload type");
    
    return 1;
}

/* ==================== Main ==================== */

int main(void) {
    TEST_SUITE_BEGIN("Handle Helper Tests");
    
    /* Type name tests */
    RUN_TEST(test_handle_type_names);
    RUN_TEST(test_handle_type_unknown);
    
    /* Handle CRUD tests */
    RUN_TEST(test_handle_store_and_retrieve);
    RUN_TEST(test_handle_wrong_type);
    RUN_TEST(test_handle_remove);
    RUN_TEST(test_handle_remove_wrong_type);
    
    /* Invalid handle tests */
    RUN_TEST(test_handle_invalid_id);
    RUN_TEST(test_handle_zero_id);
    
    /* Multi-type tests */
    RUN_TEST(test_handle_multiple_types);
    
    TEST_SUITE_END();
    
    return tests_failed > 0 ? 1 : 0;
}
