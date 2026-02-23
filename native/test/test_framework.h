/**
 * @file test_framework.h
 * @brief Simple C test framework for native module tests
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  FAIL: %s\n", message); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            printf("  FAIL: %s (expected %d, got %d)\n", message, (int)(expected), (int)(actual)); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(actual, expected, message) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            printf("  FAIL: %s (expected '%s', got '%s')\n", message, expected, actual); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_NULL(ptr, message) \
    do { \
        if ((ptr) != NULL) { \
            printf("  FAIL: %s (expected NULL)\n", message); \
            return 0; \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == NULL) { \
            printf("  FAIL: %s (expected non-NULL)\n", message); \
            return 0; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        tests_run++; \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            tests_passed++; \
            printf("  PASS\n"); \
        } else { \
            tests_failed++; \
        } \
    } while (0)

#define TEST_SUITE_BEGIN(name) \
    printf("\n=== %s ===\n\n", name)

#define TEST_SUITE_END() \
    printf("\n=== Results: %d/%d passed, %d failed ===\n\n", \
           tests_passed, tests_run, tests_failed)

#endif /* TEST_FRAMEWORK_H */
