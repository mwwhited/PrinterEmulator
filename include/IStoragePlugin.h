#ifndef ISTORAGEPLUGIN_H
#define ISTORAGEPLUGIN_H

#include <Arduino.h>
#include "HardwareConfig.h"

/**
 * Storage plugin interface for multi-storage architecture
 * Supports SD Card, EEPROM minimal filesystem, and Serial transfer
 */
class IStoragePlugin {
public:
    enum StorageType {
        STORAGE_SD_CARD,
        STORAGE_EEPROM,
        STORAGE_SERIAL,
        STORAGE_AUTO
    };
    
    virtual ~IStoragePlugin() = default;
    
    /**
     * Initialize storage plugin
     * @return STATUS_OK on success, error code on failure
     */
    virtual int initialize() = 0;
    
    /**
     * Check if storage is available and ready
     * @return true if storage is ready
     */
    virtual bool isReady() const = 0;
    
    /**
     * Get storage type
     * @return Storage type identifier
     */
    virtual StorageType getType() const = 0;
    
    /**
     * Get storage name for display
     * @return Storage name string (PROGMEM)
     */
    virtual const __FlashStringHelper* getName() const = 0;
    
    /**
     * Get available space in bytes
     * @return Available space or 0 if unknown
     */
    virtual uint32_t getAvailableSpace() const = 0;
    
    /**
     * Get total space in bytes
     * @return Total space or 0 if unknown
     */
    virtual uint32_t getTotalSpace() const = 0;
    
    /**
     * Write file to storage
     * @param filename File name (max 32 characters)
     * @param data Data buffer to write
     * @param size Number of bytes to write
     * @return Number of bytes written, or 0 on error
     */
    virtual size_t writeFile(const char* filename, const uint8_t* data, size_t size) = 0;
    
    /**
     * Read file from storage
     * @param filename File name to read
     * @param data Buffer to store read data
     * @param maxSize Maximum bytes to read
     * @return Number of bytes read, or 0 on error
     */
    virtual size_t readFile(const char* filename, uint8_t* data, size_t maxSize) = 0;
    
    /**
     * Delete file from storage
     * @param filename File name to delete
     * @return true if deletion successful
     */
    virtual bool deleteFile(const char* filename) = 0;
    
    /**
     * Check if file exists
     * @param filename File name to check
     * @return true if file exists
     */
    virtual bool fileExists(const char* filename) const = 0;
    
    /**
     * Get file size
     * @param filename File name to check
     * @return File size in bytes, or 0 if not found
     */
    virtual uint32_t getFileSize(const char* filename) const = 0;
    
    /**
     * List files in storage
     * @param filenames Array to store filenames (each MAX_FILENAME_LENGTH)
     * @param maxFiles Maximum number of files to list
     * @return Number of files found
     */
    virtual size_t listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const = 0;
    
    /**
     * Format/initialize storage
     * @return true if format successful
     */
    virtual bool format() = 0;
    
    /**
     * Get storage status information
     * @param statusBuffer Buffer to store status string
     * @param bufferSize Size of status buffer
     * @return true if status retrieved successfully
     */
    virtual bool getStatus(char* statusBuffer, size_t bufferSize) const = 0;
    
    /**
     * Validate storage integrity
     * @return true if storage is valid and accessible
     */
    virtual bool validate() const = 0;
    
    /**
     * Get memory usage of plugin
     * @return Memory usage in bytes
     */
    virtual size_t getMemoryUsage() const = 0;
};

#endif // ISTORAGEPLUGIN_H