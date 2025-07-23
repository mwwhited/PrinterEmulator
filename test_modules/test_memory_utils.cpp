#include <unity.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// Mock Arduino functions for testing
#define F(x) x

// Include MemoryUtils functions (copied for testing)
size_t safeCopy(char* dest, size_t destSize, const char* src, size_t maxCopy = 0);
bool startsWith(const char* str, size_t strLen, const char* prefix);
bool equalsIgnoreCase(const char* str1, size_t str1Len, const char* str2);
size_t safeStrlen(const char* str, size_t maxLen);
void clearBuffer(void* buffer, size_t size);
bool isValidString(const char* str, size_t maxLen);
int getAvailableRAM();

// Test setup/teardown
void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// safeCopy Tests
// ============================================================================

void test_safeCopy_normal_string() {
    char dest[20];
    const char* src = "Hello World";
    
    size_t result = safeCopy(dest, sizeof(dest), src);
    
    TEST_ASSERT_EQUAL(11, result);
    TEST_ASSERT_EQUAL_STRING("Hello World", dest);
}

void test_safeCopy_with_max_copy() {
    char dest[20];
    const char* src = "Hello World";
    
    size_t result = safeCopy(dest, sizeof(dest), src, 5);
    
    TEST_ASSERT_EQUAL(5, result);
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
}

void test_safeCopy_buffer_overflow_protection() {
    char dest[6];  // Only 6 bytes
    const char* src = "Hello World Long String";
    
    size_t result = safeCopy(dest, sizeof(dest), src);
    
    TEST_ASSERT_EQUAL(5, result);  // 6-1 for null terminator
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
    TEST_ASSERT_EQUAL('\0', dest[5]);  // Null terminated
}

void test_safeCopy_null_dest() {
    const char* src = "Hello";
    
    size_t result = safeCopy(nullptr, 10, src);
    
    TEST_ASSERT_EQUAL(0, result);
}

void test_safeCopy_null_src() {
    char dest[10];
    
    size_t result = safeCopy(dest, sizeof(dest), nullptr);
    
    TEST_ASSERT_EQUAL(0, result);
}

void test_safeCopy_zero_dest_size() {
    char dest[10];
    const char* src = "Hello";
    
    size_t result = safeCopy(dest, 0, src);
    
    TEST_ASSERT_EQUAL(0, result);
}

void test_safeCopy_empty_source() {
    char dest[10];
    const char* src = "";
    
    size_t result = safeCopy(dest, sizeof(dest), src);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_safeCopy_exact_fit() {
    char dest[6];  // Exactly "Hello" + null
    const char* src = "Hello";
    
    size_t result = safeCopy(dest, sizeof(dest), src);
    
    TEST_ASSERT_EQUAL(5, result);
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
}

// ============================================================================
// startsWith Tests  
// ============================================================================

void test_startsWith_true_exact_match() {
    const char* str = "Arduino";
    const char* prefix = "Arduino";
    
    TEST_ASSERT_TRUE(startsWith(str, 7, prefix));
}

void test_startsWith_true_partial_match() {
    const char* str = "Arduino Mega 2560";
    const char* prefix = "Arduino";
    
    TEST_ASSERT_TRUE(startsWith(str, 17, prefix));
}

void test_startsWith_false_different() {
    const char* str = "Arduino Mega";
    const char* prefix = "ESP32";
    
    TEST_ASSERT_FALSE(startsWith(str, 12, prefix));
}

void test_startsWith_false_prefix_longer() {
    const char* str = "Hi";
    const char* prefix = "Hello";
    
    TEST_ASSERT_FALSE(startsWith(str, 2, prefix));
}

void test_startsWith_null_str() {
    const char* prefix = "test";
    
    TEST_ASSERT_FALSE(startsWith(nullptr, 5, prefix));
}

void test_startsWith_null_prefix() {
    const char* str = "test";
    
    TEST_ASSERT_FALSE(startsWith(str, 4, nullptr));
}

void test_startsWith_zero_length() {
    const char* str = "test";
    const char* prefix = "t";
    
    TEST_ASSERT_FALSE(startsWith(str, 0, prefix));
}

void test_startsWith_empty_prefix() {
    const char* str = "test";
    const char* prefix = "";
    
    TEST_ASSERT_TRUE(startsWith(str, 4, prefix));
}

// ============================================================================
// equalsIgnoreCase Tests
// ============================================================================

void test_equalsIgnoreCase_true_same_case() {
    const char* str1 = "hello";
    const char* str2 = "hello";
    
    TEST_ASSERT_TRUE(equalsIgnoreCase(str1, 5, str2));
}

void test_equalsIgnoreCase_true_different_case() {
    const char* str1 = "HELLO";
    const char* str2 = "hello";
    
    TEST_ASSERT_TRUE(equalsIgnoreCase(str1, 5, str2));
}

void test_equalsIgnoreCase_true_mixed_case() {
    const char* str1 = "HeLLo";
    const char* str2 = "hEllO";
    
    TEST_ASSERT_TRUE(equalsIgnoreCase(str1, 5, str2));
}

void test_equalsIgnoreCase_false_different_strings() {
    const char* str1 = "hello";
    const char* str2 = "world";
    
    TEST_ASSERT_FALSE(equalsIgnoreCase(str1, 5, str2));
}

void test_equalsIgnoreCase_false_different_lengths() {
    const char* str1 = "hello";
    const char* str2 = "hello world";
    
    TEST_ASSERT_FALSE(equalsIgnoreCase(str1, 5, str2));
}

void test_equalsIgnoreCase_null_str1() {
    const char* str2 = "hello";
    
    TEST_ASSERT_FALSE(equalsIgnoreCase(nullptr, 5, str2));
}

void test_equalsIgnoreCase_null_str2() {
    const char* str1 = "hello";
    
    TEST_ASSERT_FALSE(equalsIgnoreCase(str1, 5, nullptr));
}

void test_equalsIgnoreCase_both_empty() {
    const char* str1 = "";
    const char* str2 = "";
    
    TEST_ASSERT_TRUE(equalsIgnoreCase(str1, 0, str2));
}

// ============================================================================
// safeStrlen Tests (if implemented)
// ============================================================================

void test_safeStrlen_normal_string() {
    const char* str = "Hello World";
    
    size_t result = safeStrlen(str, 50);
    
    TEST_ASSERT_EQUAL(11, result);
}

void test_safeStrlen_max_limit() {
    const char* str = "Hello World";
    
    size_t result = safeStrlen(str, 5);
    
    TEST_ASSERT_EQUAL(5, result);
}

void test_safeStrlen_null_string() {
    size_t result = safeStrlen(nullptr, 10);
    
    TEST_ASSERT_EQUAL(0, result);
}

void test_safeStrlen_empty_string() {
    const char* str = "";
    
    size_t result = safeStrlen(str, 10);
    
    TEST_ASSERT_EQUAL(0, result);
}

// ============================================================================
// clearBuffer Tests
// ============================================================================

void test_clearBuffer_normal() {
    uint8_t buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    clearBuffer(buffer, sizeof(buffer));
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(0, buffer[i]);
    }
}

void test_clearBuffer_null_pointer() {
    // Should not crash
    clearBuffer(nullptr, 10);
    TEST_ASSERT_TRUE(true);  // If we get here, it didn't crash
}

void test_clearBuffer_zero_size() {
    uint8_t buffer[5] = {1, 2, 3, 4, 5};
    
    clearBuffer(buffer, 0);
    
    // Buffer should be unchanged
    TEST_ASSERT_EQUAL(1, buffer[0]);
    TEST_ASSERT_EQUAL(2, buffer[1]);
}

// ============================================================================
// Memory boundary and edge case tests
// ============================================================================

void test_memory_safety_extreme_cases() {
    // Test extreme buffer sizes
    char tiny_buffer[2];
    const char* long_string = "This is a very long string that will definitely overflow";
    
    size_t result = safeCopy(tiny_buffer, sizeof(tiny_buffer), long_string);
    
    TEST_ASSERT_EQUAL(1, result);  // Only 1 char fits (2-1 for null)
    TEST_ASSERT_EQUAL_STRING("T", tiny_buffer);
}

void test_memory_alignment() {
    // Test that memory operations work with different alignments
    char buffer[16];
    clearBuffer(buffer, sizeof(buffer));
    
    // All bytes should be zero
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL(0, buffer[i]);
    }
}

// ============================================================================
// Integration tests
// ============================================================================

void test_string_operations_integration() {
    char buffer[32];
    const char* source = "ARDUINO MEGA 2560";
    
    // Copy string
    size_t len = safeCopy(buffer, sizeof(buffer), source);
    TEST_ASSERT_EQUAL(17, len);
    
    // Test if it starts with "ARDUINO"
    TEST_ASSERT_TRUE(startsWith(buffer, len, "ARDUINO"));
    
    // Test case-insensitive comparison
    TEST_ASSERT_TRUE(equalsIgnoreCase(buffer, len, "arduino mega 2560"));
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // safeCopy tests
    RUN_TEST(test_safeCopy_normal_string);
    RUN_TEST(test_safeCopy_with_max_copy);
    RUN_TEST(test_safeCopy_buffer_overflow_protection);
    RUN_TEST(test_safeCopy_null_dest);
    RUN_TEST(test_safeCopy_null_src);
    RUN_TEST(test_safeCopy_zero_dest_size);
    RUN_TEST(test_safeCopy_empty_source);
    RUN_TEST(test_safeCopy_exact_fit);
    
    // startsWith tests
    RUN_TEST(test_startsWith_true_exact_match);
    RUN_TEST(test_startsWith_true_partial_match);
    RUN_TEST(test_startsWith_false_different);
    RUN_TEST(test_startsWith_false_prefix_longer);
    RUN_TEST(test_startsWith_null_str);
    RUN_TEST(test_startsWith_null_prefix);
    RUN_TEST(test_startsWith_zero_length);
    RUN_TEST(test_startsWith_empty_prefix);
    
    // equalsIgnoreCase tests
    RUN_TEST(test_equalsIgnoreCase_true_same_case);
    RUN_TEST(test_equalsIgnoreCase_true_different_case);
    RUN_TEST(test_equalsIgnoreCase_true_mixed_case);
    RUN_TEST(test_equalsIgnoreCase_false_different_strings);
    RUN_TEST(test_equalsIgnoreCase_false_different_lengths);
    RUN_TEST(test_equalsIgnoreCase_null_str1);
    RUN_TEST(test_equalsIgnoreCase_null_str2);
    RUN_TEST(test_equalsIgnoreCase_both_empty);
    
    // safeStrlen tests
    RUN_TEST(test_safeStrlen_normal_string);
    RUN_TEST(test_safeStrlen_max_limit);
    RUN_TEST(test_safeStrlen_null_string);
    RUN_TEST(test_safeStrlen_empty_string);
    
    // clearBuffer tests
    RUN_TEST(test_clearBuffer_normal);
    RUN_TEST(test_clearBuffer_null_pointer);
    RUN_TEST(test_clearBuffer_zero_size);
    
    // Memory safety tests
    RUN_TEST(test_memory_safety_extreme_cases);
    RUN_TEST(test_memory_alignment);
    
    // Integration tests
    RUN_TEST(test_string_operations_integration);
    
    return UNITY_END();
}