/**
 * @file native/test/test_string_helpers.c
 * @brief Unit tests for string helper functions
 */

#include "test_framework.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* String validation functions (copied from string_helpers.c for standalone testing) */

static int validate_bucket_name(const char* bucket_name) {
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

static int validate_object_key(const char* object_key) {
    if (object_key == NULL || strlen(object_key) == 0) return 0;
    if (strlen(object_key) > 1024) return 0;
    return 1;
}

static char* safe_strdup(const char* str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str);
    char* copy = (char*)malloc(len + 1);
    if (copy == NULL) return NULL;
    /* copy exactly len bytes + NUL; dest is len+1 so no overflow (CWE-120) */
    memcpy(copy, str, len);
    copy[len] = '\0';
    return copy;
}

/* ==================== Tests ==================== */

int test_validate_bucket_name_minimum_length(void) {
    TEST_ASSERT_EQ(validate_bucket_name("abc"), 1, "3 chars should be valid");
    TEST_ASSERT_EQ(validate_bucket_name("ab"), 0, "2 chars should be invalid");
    TEST_ASSERT_EQ(validate_bucket_name("a"), 0, "1 char should be invalid");
    return 1;
}

int test_validate_bucket_name_maximum_length(void) {
    /* 63 characters - maximum valid length */
    char max_valid[64];
    memset(max_valid, 'a', 63);
    max_valid[63] = '\0';
    TEST_ASSERT_EQ(validate_bucket_name(max_valid), 1, "63 chars should be valid");
    
    /* 64 characters - too long */
    char too_long[65];
    memset(too_long, 'a', 64);
    too_long[64] = '\0';
    TEST_ASSERT_EQ(validate_bucket_name(too_long), 0, "64 chars should be invalid");
    
    return 1;
}

int test_validate_bucket_name_character_rules(void) {
    TEST_ASSERT_EQ(validate_bucket_name("my-bucket"), 1, "Hyphen in middle is valid");
    TEST_ASSERT_EQ(validate_bucket_name("-bucket"), 0, "Starting hyphen is invalid");
    TEST_ASSERT_EQ(validate_bucket_name("bucket-"), 0, "Ending hyphen is invalid");
    TEST_ASSERT_EQ(validate_bucket_name("my--bucket"), 1, "Double hyphen is valid");
    return 1;
}

int test_validate_bucket_name_case_sensitivity(void) {
    TEST_ASSERT_EQ(validate_bucket_name("mybucket"), 1, "Lowercase is valid");
    TEST_ASSERT_EQ(validate_bucket_name("MyBucket"), 0, "Mixed case is invalid");
    TEST_ASSERT_EQ(validate_bucket_name("MYBUCKET"), 0, "Uppercase is invalid");
    return 1;
}

int test_validate_bucket_name_special_characters(void) {
    TEST_ASSERT_EQ(validate_bucket_name("my_bucket"), 0, "Underscore is invalid");
    TEST_ASSERT_EQ(validate_bucket_name("my.bucket"), 0, "Dot is invalid");
    TEST_ASSERT_EQ(validate_bucket_name("my bucket"), 0, "Space is invalid");
    TEST_ASSERT_EQ(validate_bucket_name("my@bucket"), 0, "@ is invalid");
    return 1;
}

int test_validate_bucket_name_null_and_empty(void) {
    TEST_ASSERT_EQ(validate_bucket_name(NULL), 0, "NULL should be invalid");
    TEST_ASSERT_EQ(validate_bucket_name(""), 0, "Empty string should be invalid");
    return 1;
}

int test_validate_object_key_valid_cases(void) {
    TEST_ASSERT_EQ(validate_object_key("file.txt"), 1, "Simple filename");
    TEST_ASSERT_EQ(validate_object_key("path/to/file.txt"), 1, "Path with slashes");
    TEST_ASSERT_EQ(validate_object_key("a"), 1, "Single character");
    TEST_ASSERT_EQ(validate_object_key("My File (1).txt"), 1, "Spaces and parens");
    TEST_ASSERT_EQ(validate_object_key("文件.txt"), 1, "Unicode characters");
    return 1;
}

int test_validate_object_key_invalid_cases(void) {
    TEST_ASSERT_EQ(validate_object_key(NULL), 0, "NULL should be invalid");
    TEST_ASSERT_EQ(validate_object_key(""), 0, "Empty string should be invalid");
    return 1;
}

int test_validate_object_key_max_length(void) {
    /* 1024 characters - valid */
    char* max_valid = (char*)malloc(1025);
    memset(max_valid, 'a', 1024);
    max_valid[1024] = '\0';
    TEST_ASSERT_EQ(validate_object_key(max_valid), 1, "1024 chars should be valid");
    free(max_valid);
    
    /* 1025 characters - too long */
    char* too_long = (char*)malloc(1026);
    memset(too_long, 'a', 1025);
    too_long[1025] = '\0';
    TEST_ASSERT_EQ(validate_object_key(too_long), 0, "1025 chars should be invalid");
    free(too_long);
    
    return 1;
}

int test_safe_strdup_normal(void) {
    const char* original = "test string";
    char* copy = safe_strdup(original);
    TEST_ASSERT_NOT_NULL(copy, "Copy should not be NULL");
    TEST_ASSERT_STR_EQ(copy, original, "Copy should match original");
    free(copy);
    return 1;
}

int test_safe_strdup_null(void) {
    char* copy = safe_strdup(NULL);
    TEST_ASSERT_NULL(copy, "NULL input should return NULL");
    return 1;
}

int test_safe_strdup_empty(void) {
    char* copy = safe_strdup("");
    TEST_ASSERT_NOT_NULL(copy, "Empty string copy should not be NULL");
    TEST_ASSERT_STR_EQ(copy, "", "Empty string copy should be empty");
    free(copy);
    return 1;
}

/* ==================== Main ==================== */

int main(void) {
    TEST_SUITE_BEGIN("String Helper Tests");
    
    /* Bucket name validation tests */
    RUN_TEST(test_validate_bucket_name_minimum_length);
    RUN_TEST(test_validate_bucket_name_maximum_length);
    RUN_TEST(test_validate_bucket_name_character_rules);
    RUN_TEST(test_validate_bucket_name_case_sensitivity);
    RUN_TEST(test_validate_bucket_name_special_characters);
    RUN_TEST(test_validate_bucket_name_null_and_empty);
    
    /* Object key validation tests */
    RUN_TEST(test_validate_object_key_valid_cases);
    RUN_TEST(test_validate_object_key_invalid_cases);
    RUN_TEST(test_validate_object_key_max_length);
    
    /* String duplication tests */
    RUN_TEST(test_safe_strdup_normal);
    RUN_TEST(test_safe_strdup_null);
    RUN_TEST(test_safe_strdup_empty);
    
    TEST_SUITE_END();
    
    return tests_failed > 0 ? 1 : 0;
}
