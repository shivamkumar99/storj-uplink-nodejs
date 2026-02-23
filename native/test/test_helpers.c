/**
 * @file test_helpers.c
 * @brief Unit tests for helper functions (standalone tests only)
 * 
 * These tests don't require node_api.h - they only test pure C functions.
 */

#include "test_framework.h"
#include <string.h>
#include <ctype.h>

/* 
 * Copy of validation functions for standalone testing
 * (The actual functions are in string_helpers.c but require node_api.h)
 */

static int test_validate_bucket_name(const char* bucket_name) {
    if (bucket_name == NULL) return 0;
    
    size_t len = strlen(bucket_name);
    
    /* Length: 3-63 characters */
    if (len < 3 || len > 63) return 0;
    
    /* Must start and end with letter or number */
    if (!isalnum((unsigned char)bucket_name[0]) ||
        !isalnum((unsigned char)bucket_name[len - 1])) return 0;
    
    /* Only lowercase letters, numbers, and hyphens */
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)bucket_name[i];
        if (!islower(c) && !isdigit(c) && c != '-') {
            return 0;
        }
    }
    
    return 1;
}

static int test_validate_object_key(const char* object_key) {
    if (object_key == NULL || strlen(object_key) == 0) return 0;
    if (strlen(object_key) > 1024) return 0;
    return 1;
}

/* Handle type names for testing */
static const char* handle_type_names[] = {
    "Access", "Project", "Download", "Upload", "EncryptionKey",
    "PartUpload", "ObjectIterator", "BucketIterator", "UploadIterator", "PartIterator"
};

static const char* test_get_handle_type_name(int type) {
    if (type >= 0 && type < (int)(sizeof(handle_type_names)/sizeof(handle_type_names[0]))) {
        return handle_type_names[type];
    }
    return "Unknown";
}

static int test_validate_handle(size_t handle) {
    return handle != 0;
}

/* ==================== String Helper Tests ==================== */

int test_bucket_name_valid(void) {
    TEST_ASSERT_EQ(test_validate_bucket_name("my-bucket"), 1, "Valid bucket with hyphen");
    TEST_ASSERT_EQ(test_validate_bucket_name("bucket123"), 1, "Alphanumeric bucket");
    TEST_ASSERT_EQ(test_validate_bucket_name("abc"), 1, "Minimum length bucket");
    TEST_ASSERT_EQ(test_validate_bucket_name("test-bucket-name"), 1, "Multiple hyphens");
    return 1;
}

int test_bucket_name_invalid(void) {
    TEST_ASSERT_EQ(test_validate_bucket_name(""), 0, "Empty bucket name");
    TEST_ASSERT_EQ(test_validate_bucket_name("ab"), 0, "Too short (2 chars)");
    TEST_ASSERT_EQ(test_validate_bucket_name("MyBucket"), 0, "Uppercase letters");
    TEST_ASSERT_EQ(test_validate_bucket_name("-bucket"), 0, "Starts with hyphen");
    TEST_ASSERT_EQ(test_validate_bucket_name("bucket-"), 0, "Ends with hyphen");
    TEST_ASSERT_EQ(test_validate_bucket_name("bucket_name"), 0, "Contains underscore");
    TEST_ASSERT_EQ(test_validate_bucket_name("bucket.name"), 0, "Contains dot");
    TEST_ASSERT_EQ(test_validate_bucket_name(NULL), 0, "NULL bucket name");
    return 1;
}

int test_object_key_valid(void) {
    TEST_ASSERT_EQ(test_validate_object_key("file.txt"), 1, "Simple file");
    TEST_ASSERT_EQ(test_validate_object_key("path/to/file.txt"), 1, "Path with slashes");
    TEST_ASSERT_EQ(test_validate_object_key("a"), 1, "Single character");
    TEST_ASSERT_EQ(test_validate_object_key("My File (1).txt"), 1, "Spaces and special chars");
    return 1;
}

int test_object_key_invalid(void) {
    TEST_ASSERT_EQ(test_validate_object_key(""), 0, "Empty object key");
    TEST_ASSERT_EQ(test_validate_object_key(NULL), 0, "NULL object key");
    return 1;
}

/* ==================== Handle Helper Tests ==================== */

int test_handle_type_names(void) {
    TEST_ASSERT_STR_EQ(test_get_handle_type_name(0), "Access", "Access type name");
    TEST_ASSERT_STR_EQ(test_get_handle_type_name(1), "Project", "Project type name");
    TEST_ASSERT_STR_EQ(test_get_handle_type_name(2), "Download", "Download type name");
    TEST_ASSERT_STR_EQ(test_get_handle_type_name(3), "Upload", "Upload type name");
    TEST_ASSERT_STR_EQ(test_get_handle_type_name(999), "Unknown", "Unknown type");
    return 1;
}

int test_handle_validation(void) {
    TEST_ASSERT_EQ(test_validate_handle(0), 0, "Zero handle is invalid");
    TEST_ASSERT_EQ(test_validate_handle(1), 1, "Non-zero handle is valid");
    TEST_ASSERT_EQ(test_validate_handle(12345), 1, "Large handle is valid");
    return 1;
}

/* ==================== Main ==================== */

int main(void) {
    TEST_SUITE_BEGIN("Helper Function Tests (Standalone)");
    
    /* String helper tests */
    RUN_TEST(test_bucket_name_valid);
    RUN_TEST(test_bucket_name_invalid);
    RUN_TEST(test_object_key_valid);
    RUN_TEST(test_object_key_invalid);
    
    /* Handle helper tests */
    RUN_TEST(test_handle_type_names);
    RUN_TEST(test_handle_validation);
    
    TEST_SUITE_END();
    
    return tests_failed > 0 ? 1 : 0;
}
