#ifndef SDCARDSTRAGEPLUGIN_H
#define SDCARDSTRAGEPLUGIN_H

#include <Arduino.h>
#include <SD.h>
#include "IStoragePlugin.h"
#include "HardwareConfig.h"

/**
 * SD Card Storage Plugin
 * Implements FAT16/FAT32 filesystem support for SD cards up to 32GB
 * Supports hot-swap detection and write protection
 */
class SDCardStoragePlugin : public IStoragePlugin {
private:
    bool initialized;
    bool cardPresent;
    bool writeProtected;
    bool debugEnabled;
    
    // Card information
    uint32_t cardSize;          // Total card size in bytes
    uint32_t freeSpace;         // Available space in bytes
    
    // File operation buffer
    char pathBuffer[MAX_FILENAME_LENGTH + 8]; // Extra space for path
    
    /**
     * Check card presence and write protection
     */
    void checkCardStatus();
    
    /**
     * Update free space information
     */
    void updateFreeSpace();
    
    /**
     * Ensure directory structure exists
     * @param filename File path to check
     * @return true if directory structure is ready
     */
    bool ensureDirectoryExists(const char* filename);
    
public:
    /**
     * Constructor
     */
    SDCardStoragePlugin();
    
    /**
     * Destructor
     */
    ~SDCardStoragePlugin() override = default;
    
    // IStoragePlugin interface implementation
    int initialize() override;
    bool isReady() const override;
    StorageType getType() const override;
    const __FlashStringHelper* getName() const override;
    uint32_t getAvailableSpace() const override;
    uint32_t getTotalSpace() const override;
    size_t writeFile(const char* filename, const uint8_t* data, size_t size) override;
    size_t readFile(const char* filename, uint8_t* data, size_t maxSize) override;
    bool deleteFile(const char* filename) override;
    bool fileExists(const char* filename) const override;
    uint32_t getFileSize(const char* filename) const override;
    size_t listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const override;
    bool format() override;
    bool getStatus(char* statusBuffer, size_t bufferSize) const override;
    bool validate() const override;
    size_t getMemoryUsage() const override;
    
    /**
     * Check if card is present
     * @return true if SD card is detected
     */
    bool isCardPresent() const;
    
    /**
     * Check if card is write protected
     * @return true if write protection is enabled
     */
    bool isWriteProtected() const;
    
    /**
     * Get card type information
     * @param typeBuffer Buffer to store card type string
     * @param bufferSize Size of type buffer
     * @return true if card type retrieved
     */
    bool getCardType(char* typeBuffer, size_t bufferSize) const;
    
    /**
     * Force card detection refresh
     */
    void refreshCardStatus();
    
    /**
     * Enable/disable debug output
     * @param enabled true to enable debug output
     */
    void setDebugEnabled(bool enabled);
    
    /**
     * Create directory structure
     * @param dirPath Directory path to create
     * @return true if directory created or exists
     */
    bool createDirectory(const char* dirPath);
    
    /**
     * List files in specific directory
     * @param dirPath Directory path to list (nullptr for root)
     * @param filenames Array to store filenames
     * @param maxFiles Maximum number of files to list
     * @return Number of files found
     */
    size_t listFilesInDirectory(const char* dirPath, 
                               char filenames[][MAX_FILENAME_LENGTH], 
                               size_t maxFiles) const;
};

#endif // SDCARDSTRAGEPLUGIN_H