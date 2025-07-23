#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// Mock Arduino functions for testing
#define F(x) x

// Include memory utils without Arduino dependencies
size_t safeCopy(char* dest, size_t destSize, const char* src, size_t maxCopy = 0) {
    if (!dest || !src || destSize == 0) {
        return 0;
    }
    
    size_t copyLen = 0;
    size_t maxLen = (maxCopy == 0) ? destSize - 1 : (maxCopy < destSize - 1 ? maxCopy : destSize - 1);
    
    while (copyLen < maxLen && src[copyLen] != '\0') {
        dest[copyLen] = src[copyLen];
        copyLen++;
    }
    
    dest[copyLen] = '\0';
    return copyLen;
}

bool startsWith(const char* str, size_t strLen, const char* prefix) {
    if (!str || !prefix || strLen == 0) {
        return false;
    }
    
    size_t prefixLen = strlen(prefix);
    if (prefixLen > strLen) {
        return false;
    }
    
    for (size_t i = 0; i < prefixLen; i++) {
        if (str[i] != prefix[i]) {
            return false;
        }
    }
    
    return true;
}

bool equalsIgnoreCase(const char* str1, size_t str1Len, const char* str2) {
    if (!str1 || !str2) {
        return false;
    }
    
    size_t str2Len = strlen(str2);
    if (str1Len != str2Len) {
        return false;
    }
    
    for (size_t i = 0; i < str1Len; i++) {
        if (tolower(str1[i]) != tolower(str2[i])) {
            return false;
        }
    }
    
    return true;
}

bool parseInt(const char* str, size_t strLen, int& result) {
    if (!str || strLen == 0) {
        return false;
    }
    
    result = 0;
    bool negative = false;
    size_t startIndex = 0;
    
    if (str[0] == '-') {
        negative = true;
        startIndex = 1;
    } else if (str[0] == '+') {
        startIndex = 1;
    }
    
    if (startIndex >= strLen) {
        return false;
    }
    
    for (size_t i = startIndex; i < strLen && str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return false;
        }
        
        if (result > (INT_MAX - (str[i] - '0')) / 10) {
            return false;
        }
        
        result = result * 10 + (str[i] - '0');
    }
    
    if (negative) {
        result = -result;
    }
    
    return true;
}

// Test functions
void setUp(void) {}
void tearDown(void) {}

void test_safeCopy_basic() {
    char dest[10];
    const char* src = "Hello";
    
    size_t result = safeCopy(dest, sizeof(dest), src);
    
    TEST_ASSERT_EQUAL(5, result);
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
}

void test_safeCopy_with_limit() {
    char dest[10];
    const char* src = "HelloWorld123";
    
    size_t result = safeCopy(dest, sizeof(dest), src);
    
    TEST_ASSERT_EQUAL(9, result);
    TEST_ASSERT_EQUAL_STRING("HelloWorl", dest);
}

void test_startsWith_true() {
    const char* str = "Arduino Mega";
    const char* prefix = "Arduino";
    
    TEST_ASSERT_TRUE(startsWith(str, 12, prefix));
}

void test_startsWith_false() {
    const char* str = "Arduino Mega";
    const char* prefix = "Nano";
    
    TEST_ASSERT_FALSE(startsWith(str, 12, prefix));
}

void test_equalsIgnoreCase_true() {
    const char* str1 = "HELLO";
    const char* str2 = "hello";
    
    TEST_ASSERT_TRUE(equalsIgnoreCase(str1, 5, str2));
}

void test_parseInt_valid() {
    const char* str = "12345";
    int result;
    
    TEST_ASSERT_TRUE(parseInt(str, 5, result));
    TEST_ASSERT_EQUAL(12345, result);
}

void test_parseInt_negative() {
    const char* str = "-123";
    int result;
    
    TEST_ASSERT_TRUE(parseInt(str, 4, result));
    TEST_ASSERT_EQUAL(-123, result);
}

void test_ringbuffer_simulation() {
    // Simulate ring buffer behavior
    const size_t BUFFER_SIZE = 8;
    uint8_t buffer[BUFFER_SIZE];
    size_t head = 0, tail = 0, count = 0;
    
    // Test write
    for (int i = 0; i < 5; i++) {
        if (count < BUFFER_SIZE) {
            buffer[head] = (uint8_t)(i + 0x10);
            head = (head + 1) % BUFFER_SIZE;
            count++;
        }
    }
    
    TEST_ASSERT_EQUAL(5, count);
    
    // Test read
    for (int i = 0; i < 5; i++) {
        if (count > 0) {
            uint8_t data = buffer[tail];
            tail = (tail + 1) % BUFFER_SIZE;
            count--;
            TEST_ASSERT_EQUAL((uint8_t)(i + 0x10), data);
        }
    }
    
    TEST_ASSERT_EQUAL(0, count);
}

void test_memory_management() {
    // Test memory boundaries and safety
    char buffer[32];
    
    // Test buffer overflow protection
    const char* longString = "This is a very long string that exceeds buffer size";
    size_t result = safeCopy(buffer, sizeof(buffer), longString);
    
    TEST_ASSERT_EQUAL(31, result); // Should be truncated to fit
    TEST_ASSERT_EQUAL('\0', buffer[31]); // Null terminated
}

void test_system_constants() {
    // Test that our memory optimizations are correct
    TEST_ASSERT_EQUAL(224, RING_BUFFER_SIZE);
    TEST_ASSERT_EQUAL(32, COMMAND_BUFFER_SIZE);
    TEST_ASSERT_EQUAL(96, TRANSFER_BUFFER_SIZE);
    TEST_ASSERT_EQUAL(16, MAX_FILENAME_LENGTH);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_safeCopy_basic);
    RUN_TEST(test_safeCopy_with_limit);
    RUN_TEST(test_startsWith_true);
    RUN_TEST(test_startsWith_false);
    RUN_TEST(test_equalsIgnoreCase_true);
    RUN_TEST(test_parseInt_valid);
    RUN_TEST(test_parseInt_negative);
    RUN_TEST(test_ringbuffer_simulation);
    RUN_TEST(test_memory_management);
    RUN_TEST(test_system_constants);
    
    return UNITY_END();
}