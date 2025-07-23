#ifndef MEMORYUTILS_H
#define MEMORYUTILS_H

#include <Arduino.h>
#include <avr/pgmspace.h>

/**
 * Memory management utilities for zero-allocation architecture
 * All functions are designed for static buffer management with bounds checking
 */

/**
 * Safe string copy with explicit size limits
 * @param dest Destination buffer
 * @param destSize Size of destination buffer
 * @param src Source string (can be PROGMEM)
 * @param maxCopy Maximum bytes to copy (if 0, copy until null terminator)
 * @return Number of bytes copied (excluding null terminator)
 */
size_t safeCopy(char* dest, size_t destSize, const char* src, size_t maxCopy = 0);

/**
 * Safe string copy from PROGMEM string
 * @param dest Destination buffer
 * @param destSize Size of destination buffer
 * @param src Source PROGMEM string
 * @param maxCopy Maximum bytes to copy (if 0, copy until null terminator)
 * @return Number of bytes copied (excluding null terminator)
 */
size_t safeCopyPGM(char* dest, size_t destSize, const __FlashStringHelper* src, size_t maxCopy = 0);

/**
 * Check if string starts with specified prefix
 * @param str String to check
 * @param strLen Length of string (for bounds checking)
 * @param prefix Prefix to match
 * @return true if string starts with prefix
 */
bool startsWith(const char* str, size_t strLen, const char* prefix);

/**
 * Check if string starts with PROGMEM prefix
 * @param str String to check
 * @param strLen Length of string (for bounds checking)
 * @param prefix PROGMEM prefix to match
 * @return true if string starts with prefix
 */
bool startsWithPGM(const char* str, size_t strLen, const __FlashStringHelper* prefix);

/**
 * Case-insensitive string comparison
 * @param str1 First string
 * @param str1Len Length of first string
 * @param str2 Second string
 * @return true if strings are equal (case-insensitive)
 */
bool equalsIgnoreCase(const char* str1, size_t str1Len, const char* str2);

/**
 * Case-insensitive string comparison with PROGMEM string
 * @param str1 First string
 * @param str1Len Length of first string
 * @param str2 PROGMEM string
 * @return true if strings are equal (case-insensitive)
 */
bool equalsIgnoreCasePGM(const char* str1, size_t str1Len, const __FlashStringHelper* str2);

/**
 * Find character in string with bounds checking
 * @param str String to search
 * @param strLen Length of string
 * @param ch Character to find
 * @return Pointer to character or nullptr if not found
 */
const char* findChar(const char* str, size_t strLen, char ch);

/**
 * Convert string to integer with bounds checking
 * @param str String to convert
 * @param strLen Length of string
 * @param result Pointer to store result
 * @return true if conversion successful
 */
bool parseInt(const char* str, size_t strLen, int& result);

/**
 * Convert integer to string with bounds checking
 * @param value Integer value
 * @param dest Destination buffer
 * @param destSize Size of destination buffer
 * @return Number of characters written
 */
size_t intToString(int value, char* dest, size_t destSize);

/**
 * Append string to buffer with bounds checking
 * @param dest Destination buffer
 * @param destSize Size of destination buffer
 * @param src Source string to append
 * @return true if append successful
 */
bool appendString(char* dest, size_t destSize, const char* src);

/**
 * Append PROGMEM string to buffer with bounds checking
 * @param dest Destination buffer
 * @param destSize Size of destination buffer
 * @param src PROGMEM source string to append
 * @return true if append successful
 */
bool appendStringPGM(char* dest, size_t destSize, const __FlashStringHelper* src);

/**
 * Get length of string with maximum limit
 * @param str String to measure
 * @param maxLen Maximum length to check
 * @return String length or maxLen if longer
 */
size_t safeStrlen(const char* str, size_t maxLen);

/**
 * Clear buffer by setting all bytes to zero
 * @param buffer Buffer to clear
 * @param size Size of buffer
 */
void clearBuffer(void* buffer, size_t size);

/**
 * Check available RAM
 * @return Available RAM in bytes
 */
int getAvailableRAM();

/**
 * Memory validation for debugging
 * @param ptr Pointer to validate
 * @param size Size to check
 * @return true if memory appears valid
 */
bool validateMemory(const void* ptr, size_t size);

#endif // MEMORYUTILS_H