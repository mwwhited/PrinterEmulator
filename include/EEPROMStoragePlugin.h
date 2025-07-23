#ifndef EEPROMSTORAGEPLUGIN_H
#define EEPROMSTORAGEPLUGIN_H

#include <Arduino.h>
#include <SPI.h>
#include "IStoragePlugin.h"
#include "HardwareConfig.h"

/**
 * EEPROM Storage Plugin for W25Q128FVSG (16MB SPI Flash)
 * Implements minimal filesystem with wear leveling support
 * Uses complement-based size encoding and sector management
 */
class EEPROMStoragePlugin : public IStoragePlugin {
private:
    struct FileEntry {
        char filename[MAX_FILENAME_LENGTH];
        uint32_t startSector;      // Starting sector number
        uint32_t sizeBytes;        // File size in bytes
        uint32_t sizeComplement;   // Complement of size for validation
        uint8_t status;            // File status (active, deleted, etc.)
        uint8_t reserved[3];       // Reserved for alignment
    };
    
    static constexpr size_t FILE_ENTRY_SIZE = sizeof(FileEntry);
    static constexpr size_t MAX_FILES = 64;  // Maximum files in filesystem
    static constexpr uint32_t DIRECTORY_SECTOR = 0; // First sector for directory
    static constexpr uint32_t DATA_START_SECTOR = 1; // First data sector
    static constexpr uint32_t TOTAL_SECTORS = EEPROM_SIZE / EEPROM_SECTOR_SIZE;
    
    enum FileStatus {
        STATUS_EMPTY = 0xFF,
        STATUS_ACTIVE = 0xAA,
        STATUS_DELETED = 0x55
    };
    
    // W25Q128 Commands
    static constexpr uint8_t CMD_READ_DATA = 0x03;
    static constexpr uint8_t CMD_PAGE_PROGRAM = 0x02;
    static constexpr uint8_t CMD_SECTOR_ERASE = 0x20;
    static constexpr uint8_t CMD_WRITE_ENABLE = 0x06;
    static constexpr uint8_t CMD_READ_STATUS1 = 0x05;
    static constexpr uint8_t CMD_CHIP_ERASE = 0xC7;
    static constexpr uint8_t CMD_JEDEC_ID = 0x9F;
    
    bool initialized;
    bool debugEnabled;
    
    // Filesystem state
    FileEntry directory[MAX_FILES];
    uint32_t nextFreeSector;
    uint32_t totalFiles;
    uint32_t deletedFiles;
    
    // Operation buffers
    uint8_t sectorBuffer[EEPROM_SECTOR_SIZE];
    uint8_t pageBuffer[EEPROM_PAGE_SIZE];
    
    /**
     * Initialize SPI communication
     */
    void initSPI();
    
    /**
     * Send command to EEPROM
     * @param cmd Command byte
     */
    void sendCommand(uint8_t cmd);
    
    /**
     * Send address (24-bit) to EEPROM
     * @param address 24-bit address
     */
    void sendAddress(uint32_t address);
    
    /**
     * Wait for write operation to complete
     * @param timeoutMs Timeout in milliseconds
     * @return true if operation completed
     */
    bool waitForWriteComplete(uint32_t timeoutMs = 1000);
    
    /**
     * Enable write operations
     */
    void writeEnable();
    
    /**
     * Read data from EEPROM
     * @param address Start address
     * @param data Buffer to store read data
     * @param size Number of bytes to read
     * @return true if read successful
     */
    bool readData(uint32_t address, uint8_t* data, size_t size);
    
    /**
     * Write page to EEPROM (256 bytes max)
     * @param address Start address (must be page-aligned)
     * @param data Data to write
     * @param size Number of bytes to write (max 256)
     * @return true if write successful
     */
    bool writePage(uint32_t address, const uint8_t* data, size_t size);
    
    /**
     * Erase sector (4KB)
     * @param sectorNum Sector number to erase
     * @return true if erase successful
     */
    bool eraseSector(uint32_t sectorNum);
    
    /**
     * Load directory from EEPROM
     * @return true if directory loaded successfully
     */
    bool loadDirectory();
    
    /**
     * Save directory to EEPROM
     * @return true if directory saved successfully
     */
    bool saveDirectory();
    
    /**
     * Find file entry by name
     * @param filename File name to find
     * @return Pointer to file entry or nullptr if not found
     */
    FileEntry* findFileEntry(const char* filename);
    
    /**
     * Find empty file entry slot
     * @return Pointer to empty entry or nullptr if directory full
     */
    FileEntry* findEmptyEntry();
    
    /**
     * Allocate sectors for file
     * @param sizeBytes File size in bytes
     * @return Starting sector number or 0 if allocation failed
     */
    uint32_t allocateSectors(uint32_t sizeBytes);
    
    /**
     * Free sectors used by file
     * @param startSector Starting sector number
     * @param sizeBytes File size in bytes
     */
    void freeSectors(uint32_t startSector, uint32_t sizeBytes);
    
    /**
     * Get number of sectors needed for size
     * @param sizeBytes Size in bytes
     * @return Number of sectors needed
     */
    uint32_t getSectorCount(uint32_t sizeBytes) const;
    
    /**
     * Validate file entry integrity
     * @param entry File entry to validate
     * @return true if entry is valid
     */
    bool validateFileEntry(const FileEntry* entry) const;
    
    /**
     * Defragment filesystem (compact deleted files)
     * @return true if defragmentation successful
     */
    bool defragment();
    
public:
    /**
     * Constructor
     */
    EEPROMStoragePlugin();
    
    /**
     * Destructor
     */
    ~EEPROMStoragePlugin() override = default;
    
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
     * Get JEDEC ID for chip identification
     * @return JEDEC ID (should be 0xEF4018 for W25Q128)
     */
    uint32_t getJEDECID();
    
    /**
     * Get filesystem statistics
     * @param totalFiles_ Total number of files
     * @param deletedFiles_ Number of deleted files
     * @param fragmentation Fragmentation percentage (0-100)
     */
    void getFilesystemStats(uint32_t& totalFiles_, uint32_t& deletedFiles_, uint8_t& fragmentation) const;
    
    /**
     * Enable/disable debug output
     * @param enabled true to enable debug output
     */
    void setDebugEnabled(bool enabled);
    
    /**
     * Perform filesystem check and repair
     * @return true if filesystem is healthy or repaired
     */
    bool fsck();
    
    /**
     * Get wear leveling statistics
     * @param minEraseCount Minimum erase count
     * @param maxEraseCount Maximum erase count
     * @param avgEraseCount Average erase count
     */
    void getWearStats(uint32_t& minEraseCount, uint32_t& maxEraseCount, uint32_t& avgEraseCount) const;
};

#endif // EEPROMSTORAGEPLUGIN_H