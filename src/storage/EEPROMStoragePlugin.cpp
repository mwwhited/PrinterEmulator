#include "EEPROMStoragePlugin.h"
#include "MemoryUtils.h"

EEPROMStoragePlugin::EEPROMStoragePlugin() 
    : initialized(false), debugEnabled(false), nextFreeSector(DATA_START_SECTOR),
      totalFiles(0), deletedFiles(0) {
    clearBuffer(directory, sizeof(directory));
    clearBuffer(sectorBuffer, sizeof(sectorBuffer));
    clearBuffer(pageBuffer, sizeof(pageBuffer));
}

int EEPROMStoragePlugin::initialize() {
    if (initialized) {
        return STATUS_OK;
    }
    
    // Initialize SPI
    initSPI();
    
    // Check JEDEC ID to verify chip presence
    uint32_t jedecId = getJEDECID();
    if (jedecId == 0x000000 || jedecId == 0xFFFFFF) {
        // No chip detected or communication failure
        Serial.print(F("EEPROMStoragePlugin: No chip detected (JEDEC: 0x"));
        Serial.print(jedecId, HEX);
        Serial.println(F(") - using SD card or Serial storage"));
        return STATUS_ERROR;
    }
    
    if (jedecId != 0xEF4018) {
        // Different chip detected - warn but try to continue
        Serial.print(F("EEPROMStoragePlugin: Non-standard chip (JEDEC: 0x"));
        Serial.print(jedecId, HEX);
        Serial.println(F(") - may not work correctly"));
        // Continue anyway - might be compatible
    }
    
    // Load directory from EEPROM
    if (!loadDirectory()) {
        if (debugEnabled) {
            Serial.println(F("EEPROMStoragePlugin: Failed to load directory, formatting..."));
        }
        if (!format()) {
            return STATUS_ERROR;
        }
    }
    
    initialized = true;
    
    if (debugEnabled) {
        Serial.print(F("EEPROMStoragePlugin: Initialized, "));
        Serial.print(totalFiles);
        Serial.print(F(" files, "));
        Serial.print(deletedFiles);
        Serial.println(F(" deleted"));
    }
    
    return STATUS_OK;
}

bool EEPROMStoragePlugin::isReady() const {
    return initialized;
}

IStoragePlugin::StorageType EEPROMStoragePlugin::getType() const {
    return STORAGE_EEPROM;
}

const __FlashStringHelper* EEPROMStoragePlugin::getName() const {
    return F("EEPROM");
}

uint32_t EEPROMStoragePlugin::getAvailableSpace() const {
    if (!initialized) {
        return 0;
    }
    
    uint32_t usedSectors = nextFreeSector - DATA_START_SECTOR;
    uint32_t availableSectors = TOTAL_SECTORS - nextFreeSector;
    
    return availableSectors * EEPROM_SECTOR_SIZE;
}

uint32_t EEPROMStoragePlugin::getTotalSpace() const {
    // Total space minus directory sector
    return (TOTAL_SECTORS - 1) * EEPROM_SECTOR_SIZE;
}

void EEPROMStoragePlugin::initSPI() {
    pinMode(EEPROM_CS_PIN, OUTPUT);
    digitalWrite(EEPROM_CS_PIN, HIGH);
    
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4); // 4MHz for W25Q128
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
}

void EEPROMStoragePlugin::sendCommand(uint8_t cmd) {
    SPI.transfer(cmd);
}

void EEPROMStoragePlugin::sendAddress(uint32_t address) {
    SPI.transfer((address >> 16) & 0xFF);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
}

bool EEPROMStoragePlugin::waitForWriteComplete(uint32_t timeoutMs) {
    uint32_t startTime = millis();
    
    while (millis() - startTime < timeoutMs) {
        digitalWrite(EEPROM_CS_PIN, LOW);
        sendCommand(CMD_READ_STATUS1);
        uint8_t status = SPI.transfer(0x00);
        digitalWrite(EEPROM_CS_PIN, HIGH);
        
        if ((status & 0x01) == 0) { // WIP bit cleared
            return true;
        }
        
        delay(1);
    }
    
    return false;
}

void EEPROMStoragePlugin::writeEnable() {
    digitalWrite(EEPROM_CS_PIN, LOW);
    sendCommand(CMD_WRITE_ENABLE);
    digitalWrite(EEPROM_CS_PIN, HIGH);
}

bool EEPROMStoragePlugin::readData(uint32_t address, uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return false;
    }
    
    digitalWrite(EEPROM_CS_PIN, LOW);
    sendCommand(CMD_READ_DATA);
    sendAddress(address);
    
    for (size_t i = 0; i < size; i++) {
        data[i] = SPI.transfer(0x00);
    }
    
    digitalWrite(EEPROM_CS_PIN, HIGH);
    return true;
}

bool EEPROMStoragePlugin::writePage(uint32_t address, const uint8_t* data, size_t size) {
    if (!data || size == 0 || size > EEPROM_PAGE_SIZE) {
        return false;
    }
    
    writeEnable();
    
    digitalWrite(EEPROM_CS_PIN, LOW);
    sendCommand(CMD_PAGE_PROGRAM);
    sendAddress(address);
    
    for (size_t i = 0; i < size; i++) {
        SPI.transfer(data[i]);
    }
    
    digitalWrite(EEPROM_CS_PIN, HIGH);
    
    return waitForWriteComplete();
}

bool EEPROMStoragePlugin::eraseSector(uint32_t sectorNum) {
    if (sectorNum >= TOTAL_SECTORS) {
        return false;
    }
    
    uint32_t address = sectorNum * EEPROM_SECTOR_SIZE;
    
    writeEnable();
    
    digitalWrite(EEPROM_CS_PIN, LOW);
    sendCommand(CMD_SECTOR_ERASE);
    sendAddress(address);
    digitalWrite(EEPROM_CS_PIN, HIGH);
    
    return waitForWriteComplete(5000); // Sector erase can take up to 3s
}

bool EEPROMStoragePlugin::loadDirectory() {
    // Read directory from first sector
    if (!readData(0, (uint8_t*)directory, sizeof(directory))) {
        return false;
    }
    
    // Count active and deleted files
    totalFiles = 0;
    deletedFiles = 0;
    nextFreeSector = DATA_START_SECTOR;
    
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (directory[i].status == STATUS_ACTIVE) {
            if (validateFileEntry(&directory[i])) {
                totalFiles++;
                uint32_t fileEndSector = directory[i].startSector + getSectorCount(directory[i].sizeBytes);
                if (fileEndSector > nextFreeSector) {
                    nextFreeSector = fileEndSector;
                }
            } else {
                // Invalid entry, mark as deleted
                directory[i].status = STATUS_DELETED;
                deletedFiles++;
            }
        } else if (directory[i].status == STATUS_DELETED) {
            deletedFiles++;
        }
    }
    
    return true;
}

bool EEPROMStoragePlugin::saveDirectory() {
    // Erase directory sector
    if (!eraseSector(DIRECTORY_SECTOR)) {
        return false;
    }
    
    // Write directory in pages
    uint32_t address = 0;
    const uint8_t* dirData = (const uint8_t*)directory;
    size_t remaining = sizeof(directory);
    
    while (remaining > 0) {
        size_t pageSize = min(remaining, (size_t)EEPROM_PAGE_SIZE);
        
        if (!writePage(address, dirData, pageSize)) {
            return false;
        }
        
        address += pageSize;
        dirData += pageSize;
        remaining -= pageSize;
    }
    
    return true;
}

EEPROMStoragePlugin::FileEntry* EEPROMStoragePlugin::findFileEntry(const char* filename) {
    if (!filename) {
        return nullptr;
    }
    
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (directory[i].status == STATUS_ACTIVE &&
            equalsIgnoreCase(directory[i].filename, MAX_FILENAME_LENGTH, filename)) {
            return &directory[i];
        }
    }
    
    return nullptr;
}

const EEPROMStoragePlugin::FileEntry* EEPROMStoragePlugin::findFileEntry(const char* filename) const {
    if (!filename) {
        return nullptr;
    }
    
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (directory[i].status == STATUS_ACTIVE &&
            equalsIgnoreCase(directory[i].filename, MAX_FILENAME_LENGTH, filename)) {
            return &directory[i];
        }
    }
    
    return nullptr;
}

EEPROMStoragePlugin::FileEntry* EEPROMStoragePlugin::findEmptyEntry() {
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (directory[i].status == STATUS_EMPTY || directory[i].status == STATUS_DELETED) {
            return &directory[i];
        }
    }
    
    return nullptr;
}

uint32_t EEPROMStoragePlugin::allocateSectors(uint32_t sizeBytes) {
    uint32_t sectorsNeeded = getSectorCount(sizeBytes);
    
    if (nextFreeSector + sectorsNeeded > TOTAL_SECTORS) {
        // Try defragmentation
        if (defragment()) {
            if (nextFreeSector + sectorsNeeded > TOTAL_SECTORS) {
                return 0; // Still not enough space
            }
        } else {
            return 0; // Defragmentation failed
        }
    }
    
    uint32_t startSector = nextFreeSector;
    nextFreeSector += sectorsNeeded;
    
    return startSector;
}

void EEPROMStoragePlugin::freeSectors(uint32_t startSector, uint32_t sizeBytes) {
    uint32_t sectorsToFree = getSectorCount(sizeBytes);
    
    // Erase sectors (this is a simplified approach)
    for (uint32_t i = 0; i < sectorsToFree; i++) {
        eraseSector(startSector + i);
    }
}

uint32_t EEPROMStoragePlugin::getSectorCount(uint32_t sizeBytes) const {
    return (sizeBytes + EEPROM_SECTOR_SIZE - 1) / EEPROM_SECTOR_SIZE;
}

bool EEPROMStoragePlugin::validateFileEntry(const FileEntry* entry) const {
    if (!entry) {
        return false;
    }
    
    // Check size complement
    if (entry->sizeBytes != (~entry->sizeComplement)) {
        return false;
    }
    
    // Check sector bounds
    if (entry->startSector < DATA_START_SECTOR || 
        entry->startSector >= TOTAL_SECTORS) {
        return false;
    }
    
    // Check filename validity
    if (safeStrlen(entry->filename, MAX_FILENAME_LENGTH) == 0) {
        return false;
    }
    
    return true;
}

size_t EEPROMStoragePlugin::writeFile(const char* filename, const uint8_t* data, size_t size) {
    if (!initialized || !filename || !data || size == 0) {
        return 0;
    }
    
    // Check if file already exists
    FileEntry* existingEntry = findFileEntry(filename);
    if (existingEntry) {
        // Delete existing file first
        deleteFile(filename);
    }
    
    // Find empty directory entry
    FileEntry* entry = findEmptyEntry();
    if (!entry) {
        if (debugEnabled) {
            Serial.println(F("EEPROMStoragePlugin: Directory full"));
        }
        return 0;
    }
    
    // Allocate sectors
    uint32_t startSector = allocateSectors(size);
    if (startSector == 0) {
        if (debugEnabled) {
            Serial.println(F("EEPROMStoragePlugin: No space available"));
        }
        return 0;
    }
    
    // Write data to allocated sectors
    uint32_t address = startSector * EEPROM_SECTOR_SIZE;
    const uint8_t* srcData = data;
    size_t remaining = size;
    size_t totalWritten = 0;
    
    while (remaining > 0) {
        size_t pageSize = min(remaining, (size_t)EEPROM_PAGE_SIZE);
        
        if (!writePage(address + totalWritten, srcData, pageSize)) {
            if (debugEnabled) {
                Serial.println(F("EEPROMStoragePlugin: Write failed"));
            }
            // Free allocated sectors
            freeSectors(startSector, size);
            return 0;
        }
        
        srcData += pageSize;
        remaining -= pageSize;
        totalWritten += pageSize;
    }
    
    // Update directory entry
    safeCopy(entry->filename, MAX_FILENAME_LENGTH, filename);
    entry->startSector = startSector;
    entry->sizeBytes = size;
    entry->sizeComplement = ~size;
    entry->status = STATUS_ACTIVE;
    
    // Save directory
    if (!saveDirectory()) {
        if (debugEnabled) {
            Serial.println(F("EEPROMStoragePlugin: Failed to save directory"));
        }
        freeSectors(startSector, size);
        return 0;
    }
    
    totalFiles++;
    
    if (debugEnabled) {
        Serial.print(F("EEPROMStoragePlugin: Wrote file "));
        Serial.print(filename);
        Serial.print(F(" ("));
        Serial.print(size);
        Serial.println(F(" bytes)"));
    }
    
    return size;
}

size_t EEPROMStoragePlugin::readFile(const char* filename, uint8_t* data, size_t maxSize) {
    if (!initialized || !filename || !data || maxSize == 0) {
        return 0;
    }
    
    FileEntry* entry = findFileEntry(filename);
    if (!entry) {
        return 0;
    }
    
    size_t bytesToRead = min(entry->sizeBytes, maxSize);
    uint32_t address = entry->startSector * EEPROM_SECTOR_SIZE;
    
    if (!readData(address, data, bytesToRead)) {
        return 0;
    }
    
    return bytesToRead;
}

bool EEPROMStoragePlugin::deleteFile(const char* filename) {
    if (!initialized || !filename) {
        return false;
    }
    
    FileEntry* entry = findFileEntry(filename);
    if (!entry) {
        return false;
    }
    
    // Mark entry as deleted
    entry->status = STATUS_DELETED;
    totalFiles--;
    deletedFiles++;
    
    // Save directory
    if (!saveDirectory()) {
        return false;
    }
    
    if (debugEnabled) {
        Serial.print(F("EEPROMStoragePlugin: Deleted file "));
        Serial.println(filename);
    }
    
    return true;
}

bool EEPROMStoragePlugin::fileExists(const char* filename) const {
    return findFileEntry(filename) != nullptr;
}

uint32_t EEPROMStoragePlugin::getFileSize(const char* filename) const {
    const FileEntry* entry = findFileEntry(filename);
    return entry ? entry->sizeBytes : 0;
}

size_t EEPROMStoragePlugin::listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const {
    if (!filenames || maxFiles == 0) {
        return 0;
    }
    
    size_t fileCount = 0;
    
    for (size_t i = 0; i < MAX_FILES && fileCount < maxFiles; i++) {
        if (directory[i].status == STATUS_ACTIVE) {
            safeCopy(filenames[fileCount], MAX_FILENAME_LENGTH, directory[i].filename);
            fileCount++;
        }
    }
    
    return fileCount;
}

bool EEPROMStoragePlugin::format() {
    if (debugEnabled) {
        Serial.println(F("EEPROMStoragePlugin: Formatting filesystem..."));
    }
    
    // Clear directory
    clearBuffer(directory, sizeof(directory));
    for (size_t i = 0; i < MAX_FILES; i++) {
        directory[i].status = STATUS_EMPTY;
    }
    
    // Reset counters
    totalFiles = 0;
    deletedFiles = 0;
    nextFreeSector = DATA_START_SECTOR;
    
    // Save empty directory
    if (!saveDirectory()) {
        return false;
    }
    
    if (debugEnabled) {
        Serial.println(F("EEPROMStoragePlugin: Format complete"));
    }
    
    return true;
}

bool EEPROMStoragePlugin::getStatus(char* statusBuffer, size_t bufferSize) const {
    if (!statusBuffer || bufferSize == 0) {
        return false;
    }
    
    safeCopy(statusBuffer, bufferSize, "EEPROM: ");
    
    if (!initialized) {
        appendString(statusBuffer, bufferSize, "Not initialized");
    } else {
        appendString(statusBuffer, bufferSize, "Ready");
        
        char info[32];
        snprintf(info, sizeof(info), " (%lu files)", totalFiles);
        appendString(statusBuffer, bufferSize, info);
    }
    
    return true;
}

bool EEPROMStoragePlugin::validate() const {
    return initialized;
}

size_t EEPROMStoragePlugin::getMemoryUsage() const {
    return sizeof(*this);
}

uint32_t EEPROMStoragePlugin::getJEDECID() {
    digitalWrite(EEPROM_CS_PIN, LOW);
    sendCommand(CMD_JEDEC_ID);
    
    uint32_t id = 0;
    id |= ((uint32_t)SPI.transfer(0x00)) << 16;
    id |= ((uint32_t)SPI.transfer(0x00)) << 8;
    id |= SPI.transfer(0x00);
    
    digitalWrite(EEPROM_CS_PIN, HIGH);
    
    return id;
}

void EEPROMStoragePlugin::getFilesystemStats(uint32_t& totalFiles_, uint32_t& deletedFiles_, uint8_t& fragmentation) const {
    totalFiles_ = totalFiles;
    deletedFiles_ = deletedFiles;
    
    // Calculate fragmentation as percentage of deleted files
    if (totalFiles + deletedFiles > 0) {
        fragmentation = (deletedFiles * 100) / (totalFiles + deletedFiles);
    } else {
        fragmentation = 0;
    }
}

void EEPROMStoragePlugin::setDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

bool EEPROMStoragePlugin::defragment() {
    // This is a simplified defragmentation - in practice, this would be more complex
    if (debugEnabled) {
        Serial.println(F("EEPROMStoragePlugin: Defragmentation not implemented"));
    }
    return false;
}

bool EEPROMStoragePlugin::fsck() {
    if (!initialized) {
        return false;
    }
    
    bool hasErrors = false;
    
    // Check all file entries for validity
    for (size_t i = 0; i < MAX_FILES; i++) {
        if (directory[i].status == STATUS_ACTIVE) {
            if (!validateFileEntry(&directory[i])) {
                if (debugEnabled) {
                    Serial.print(F("EEPROMStoragePlugin: Invalid entry: "));
                    Serial.println(directory[i].filename);
                }
                directory[i].status = STATUS_DELETED;
                hasErrors = true;
            }
        }
    }
    
    if (hasErrors) {
        saveDirectory();
        loadDirectory(); // Recalculate statistics
    }
    
    return !hasErrors;
}

void EEPROMStoragePlugin::getWearStats(uint32_t& minEraseCount, uint32_t& maxEraseCount, uint32_t& avgEraseCount) const {
    // Wear leveling statistics would require tracking erase counts per sector
    // This is a simplified implementation
    minEraseCount = 0;
    maxEraseCount = 0;
    avgEraseCount = 0;
}