#include "MemoryUtils.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>

size_t safeCopy(char* dest, size_t destSize, const char* src, size_t maxCopy) {
    if (!dest || !src || destSize == 0) {
        return 0;
    }
    
    size_t copyLen = 0;
    size_t maxLen = (maxCopy == 0) ? destSize - 1 : min(maxCopy, destSize - 1);
    
    while (copyLen < maxLen && src[copyLen] != '\0') {
        dest[copyLen] = src[copyLen];
        copyLen++;
    }
    
    dest[copyLen] = '\0';
    return copyLen;
}

size_t safeCopyPGM(char* dest, size_t destSize, const __FlashStringHelper* src, size_t maxCopy) {
    if (!dest || !src || destSize == 0) {
        return 0;
    }
    
    const char* srcPtr = reinterpret_cast<const char*>(src);
    size_t copyLen = 0;
    size_t maxLen = (maxCopy == 0) ? destSize - 1 : min(maxCopy, destSize - 1);
    
    while (copyLen < maxLen) {
        char ch = pgm_read_byte(srcPtr + copyLen);
        if (ch == '\0') break;
        dest[copyLen] = ch;
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

bool startsWithPGM(const char* str, size_t strLen, const __FlashStringHelper* prefix) {
    if (!str || !prefix || strLen == 0) {
        return false;
    }
    
    const char* prefixPtr = reinterpret_cast<const char*>(prefix);
    size_t i = 0;
    
    while (i < strLen) {
        char prefixChar = pgm_read_byte(prefixPtr + i);
        if (prefixChar == '\0') {
            return true; // End of prefix reached
        }
        if (str[i] != prefixChar) {
            return false;
        }
        i++;
    }
    
    // Check if we've reached end of prefix
    return pgm_read_byte(prefixPtr + i) == '\0';
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

bool equalsIgnoreCasePGM(const char* str1, size_t str1Len, const __FlashStringHelper* str2) {
    if (!str1 || !str2) {
        return false;
    }
    
    const char* str2Ptr = reinterpret_cast<const char*>(str2);
    
    for (size_t i = 0; i < str1Len; i++) {
        char str2Char = pgm_read_byte(str2Ptr + i);
        if (str2Char == '\0') {
            return false; // str2 is shorter than str1
        }
        if (tolower(str1[i]) != tolower(str2Char)) {
            return false;
        }
    }
    
    // Check if str2 has more characters
    return pgm_read_byte(str2Ptr + str1Len) == '\0';
}

const char* findChar(const char* str, size_t strLen, char ch) {
    if (!str) {
        return nullptr;
    }
    
    for (size_t i = 0; i < strLen; i++) {
        if (str[i] == ch) {
            return &str[i];
        }
        if (str[i] == '\0') {
            break;
        }
    }
    
    return nullptr;
}

bool parseInt(const char* str, size_t strLen, int& result) {
    if (!str || strLen == 0) {
        return false;
    }
    
    result = 0;
    bool negative = false;
    size_t startIndex = 0;
    
    // Handle negative sign
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
        
        // Check for overflow
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

size_t intToString(int value, char* dest, size_t destSize) {
    if (!dest || destSize == 0) {
        return 0;
    }
    
    if (destSize == 1) {
        dest[0] = '\0';
        return 0;
    }
    
    bool negative = (value < 0);
    if (negative) {
        value = -value;
    }
    
    // Handle zero case
    if (value == 0) {
        if (destSize >= 2) {
            dest[0] = '0';
            dest[1] = '\0';
            return 1;
        }
        dest[0] = '\0';
        return 0;
    }
    
    // Convert digits in reverse order
    size_t len = 0;
    int temp = value;
    while (temp > 0 && len < destSize - 1) {
        dest[len] = '0' + (temp % 10);
        temp /= 10;
        len++;
    }
    
    // Add negative sign if needed and space available
    if (negative && len < destSize - 1) {
        dest[len] = '-';
        len++;
    }
    
    // Reverse the string
    for (size_t i = 0; i < len / 2; i++) {
        char temp_char = dest[i];
        dest[i] = dest[len - 1 - i];
        dest[len - 1 - i] = temp_char;
    }
    
    dest[len] = '\0';
    return len;
}

bool appendString(char* dest, size_t destSize, const char* src) {
    if (!dest || !src || destSize == 0) {
        return false;
    }
    
    size_t destLen = safeStrlen(dest, destSize);
    if (destLen >= destSize - 1) {
        return false; // No space for additional characters
    }
    
    size_t remainingSpace = destSize - destLen - 1;
    size_t copied = safeCopy(dest + destLen, remainingSpace + 1, src);
    
    return copied > 0;
}

bool appendStringPGM(char* dest, size_t destSize, const __FlashStringHelper* src) {
    if (!dest || !src || destSize == 0) {
        return false;
    }
    
    size_t destLen = safeStrlen(dest, destSize);
    if (destLen >= destSize - 1) {
        return false; // No space for additional characters
    }
    
    size_t remainingSpace = destSize - destLen - 1;
    size_t copied = safeCopyPGM(dest + destLen, remainingSpace + 1, src);
    
    return copied > 0;
}

size_t safeStrlen(const char* str, size_t maxLen) {
    if (!str) {
        return 0;
    }
    
    size_t len = 0;
    while (len < maxLen && str[len] != '\0') {
        len++;
    }
    
    return len;
}

void clearBuffer(void* buffer, size_t size) {
    if (buffer && size > 0) {
        memset(buffer, 0, size);
    }
}

int getAvailableRAM() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

bool validateMemory(const void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return false;
    }
    
    // Basic validation - check if pointer is in reasonable range
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    
    // Check if address is in RAM range (very basic check)
    // Arduino Mega 2560 RAM is typically at 0x200-0x21FF
    if (addr < 0x200 || addr > 0x21FF) {
        // Could be in Flash memory (0x0000-0x3FFFF) which is also valid for read-only data
        if (addr > 0x3FFFF) {
            return false;
        }
    }
    
    return true;
}