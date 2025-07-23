#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <Arduino.h>
#include "IComponent.h"
#include "IStoragePlugin.h"
#include "HardwareConfig.h"

// Forward declarations for storage plugins
class SDCardStoragePlugin;
class EEPROMStoragePlugin;
class SerialStoragePlugin;

/**
 * FileSystemManager - Multi-storage file system manager
 * Implements plugin architecture for SD Card, EEPROM, and Serial transfer
 * Provides unified interface for file operations across storage types
 */
class FileSystemManager : public IComponent {
private:
    // Storage plugins
    SDCardStoragePlugin* sdCardPlugin;
    EEPROMStoragePlugin* eepromPlugin;
    SerialStoragePlugin* serialPlugin;
    
    // Current active storage
    IStoragePlugin* currentStorage;
    IStoragePlugin::StorageType currentStorageType;
    
    // State management
    bool initialized;
    bool debugEnabled;
    
    // File operation buffers
    uint8_t transferBuffer[TRANSFER_BUFFER_SIZE];
    char filenameBuffer[MAX_FILENAME_LENGTH];
    
    // Statistics
    uint32_t totalFilesWritten;
    uint32_t totalBytesWritten;
    uint32_t totalFilesRead;
    uint32_t totalBytesRead;
    
    /**
     * Get plugin by storage type
     * @param type Storage type
     * @return Pointer to plugin or nullptr
     */
    IStoragePlugin* getPluginByType(IStoragePlugin::StorageType type);
    
    /**
     * Auto-detect best available storage
     * @return Best available storage type
     */
    IStoragePlugin::StorageType autoDetectStorage();
    
    /**
     * Validate filename format
     * @param filename Filename to validate
     * @return true if filename is valid
     */
    bool isValidFilename(const char* filename) const;
    
    /**
     * Generate unique filename with timestamp
     * @param prefix Filename prefix
     * @param extension File extension (including dot)
     * @param dest Buffer to store generated filename
     * @param destSize Size of destination buffer
     * @return true if filename generated successfully
     */
    bool generateUniqueFilename(const char* prefix, const char* extension, 
                               char* dest, size_t destSize);
    
public:
    /**
     * Constructor
     */
    FileSystemManager();
    
    /**
     * Destructor
     */
    ~FileSystemManager() override = default;
    
    // IComponent interface implementation
    int initialize() override;
    int update() override;
    int getStatus() const override;
    const __FlashStringHelper* getName() const override;
    bool validate() const override;
    int reset() override;
    size_t getMemoryUsage() const override;
    void setDebugEnabled(bool enabled) override;
    bool isDebugEnabled() const override;
    
    /**
     * Register storage plugins
     * @param sdPlugin SD card storage plugin
     * @param eepromPlugin EEPROM storage plugin
     * @param serialPlugin Serial storage plugin
     */
    void registerPlugins(SDCardStoragePlugin* sdPlugin, 
                        EEPROMStoragePlugin* eepromPlugin,
                        SerialStoragePlugin* serialPlugin);
    
    /**
     * Set active storage type
     * @param type Storage type to use
     * @return true if storage type set successfully
     */
    bool setStorageType(IStoragePlugin::StorageType type);
    
    /**
     * Get current storage type
     * @return Current storage type
     */
    IStoragePlugin::StorageType getCurrentStorageType() const;
    
    /**
     * Get current storage name
     * @return Storage name string (PROGMEM)
     */
    const __FlashStringHelper* getCurrentStorageName() const;
    
    /**
     * Check if current storage is ready
     * @return true if current storage is ready
     */
    bool isStorageReady() const;
    
    /**
     * Write file to current storage
     * @param filename File name (will be validated)
     * @param data Data buffer to write
     * @param size Number of bytes to write
     * @return Number of bytes written, or 0 on error
     */
    size_t writeFile(const char* filename, const uint8_t* data, size_t size);
    
    /**
     * Write file with auto-generated filename
     * @param prefix Filename prefix
     * @param extension File extension
     * @param data Data buffer to write
     * @param size Number of bytes to write
     * @param generatedName Buffer to store generated filename (optional)
     * @param nameBufferSize Size of filename buffer
     * @return Number of bytes written, or 0 on error
     */
    size_t writeFileAuto(const char* prefix, const char* extension,
                        const uint8_t* data, size_t size,
                        char* generatedName = nullptr, size_t nameBufferSize = 0);
    
    /**
     * Read file from current storage
     * @param filename File name to read
     * @param data Buffer to store read data
     * @param maxSize Maximum bytes to read
     * @return Number of bytes read, or 0 on error
     */
    size_t readFile(const char* filename, uint8_t* data, size_t maxSize);
    
    /**
     * Copy file between storage types
     * @param filename File name to copy
     * @param sourceType Source storage type
     * @param destType Destination storage type
     * @return true if copy successful
     */
    bool copyFile(const char* filename, IStoragePlugin::StorageType sourceType,
                  IStoragePlugin::StorageType destType);
    
    /**
     * Delete file from current storage
     * @param filename File name to delete
     * @return true if deletion successful
     */
    bool deleteFile(const char* filename);
    
    /**
     * Check if file exists in current storage
     * @param filename File name to check
     * @return true if file exists
     */
    bool fileExists(const char* filename) const;
    
    /**
     * Get file size from current storage
     * @param filename File name to check
     * @return File size in bytes, or 0 if not found
     */
    uint32_t getFileSize(const char* filename) const;
    
    /**
     * List files in current storage
     * @param filenames Array to store filenames
     * @param maxFiles Maximum number of files to list
     * @return Number of files found
     */
    size_t listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const;
    
    /**
     * Get storage space information
     * @param available Available space in bytes
     * @param total Total space in bytes
     * @return true if information retrieved successfully
     */
    bool getStorageSpace(uint32_t& available, uint32_t& total) const;
    
    /**
     * Get file operation statistics
     * @param filesWritten Total files written
     * @param bytesWritten Total bytes written
     * @param filesRead Total files read
     * @param bytesRead Total bytes read
     */
    void getStatistics(uint32_t& filesWritten, uint32_t& bytesWritten,
                      uint32_t& filesRead, uint32_t& bytesRead) const;
    
    /**
     * Format current storage
     * @return true if format successful
     */
    bool formatStorage();
    
    /**
     * Get current storage status
     * @param statusBuffer Buffer to store status string
     * @param bufferSize Size of status buffer
     * @return true if status retrieved successfully
     */
    bool getStorageStatus(char* statusBuffer, size_t bufferSize) const;
    
    /**
     * Test write operation
     * @param testData Optional test data (if nullptr, generates test pattern)
     * @param testSize Size of test data
     * @return true if test write successful
     */
    bool testWrite(const uint8_t* testData = nullptr, size_t testSize = 0);
    
    /**
     * Validate all storage plugins
     * @return true if all plugins are valid
     */
    bool validateAllStorages() const;
    
    /**
     * Get plugin by storage type (for advanced operations)
     * @param type Storage type
     * @return Pointer to plugin or nullptr
     */
    IStoragePlugin* getPlugin(IStoragePlugin::StorageType type);
};

#endif // FILESYSTEMMANAGER_H