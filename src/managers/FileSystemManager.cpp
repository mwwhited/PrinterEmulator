#include "FileSystemManager.h"
#include "MemoryUtils.h"
#include "ServiceLocator.h"

// Include storage plugins (will be created)
#include "SDCardStoragePlugin.h"
#include "EEPROMStoragePlugin.h"
#include "SerialStoragePlugin.h"

FileSystemManager::FileSystemManager() 
    : sdCardPlugin(nullptr), eepromPlugin(nullptr), serialPlugin(nullptr),
      currentStorage(nullptr), currentStorageType(IStoragePlugin::STORAGE_AUTO),
      initialized(false), debugEnabled(false),
      totalFilesWritten(0), totalBytesWritten(0), 
      totalFilesRead(0), totalBytesRead(0) {
    clearBuffer(transferBuffer, TRANSFER_BUFFER_SIZE);
    clearBuffer(filenameBuffer, MAX_FILENAME_LENGTH);
}

int FileSystemManager::initialize() {
    if (initialized) {
        return STATUS_OK;
    }
    
    // Check if plugins are registered
    if (!sdCardPlugin || !eepromPlugin || !serialPlugin) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Plugins not registered"));
        }
        return STATUS_ERROR;
    }
    
    // Initialize all plugins with debugging
    int result;
    
    Serial.println(F("FileSystemManager: Initializing SD card plugin..."));
    result = sdCardPlugin->initialize();
    if (result != STATUS_OK) {
        Serial.println(F("FileSystemManager: SD card plugin init failed"));
    } else {
        Serial.println(F("FileSystemManager: SD card plugin OK"));
    }
    
    Serial.println(F("FileSystemManager: Initializing EEPROM plugin..."));
    result = eepromPlugin->initialize();
    if (result != STATUS_OK) {
        Serial.println(F("FileSystemManager: EEPROM plugin init failed"));
    } else {
        Serial.println(F("FileSystemManager: EEPROM plugin OK"));
    }
    
    Serial.println(F("FileSystemManager: Initializing Serial plugin..."));
    result = serialPlugin->initialize();
    if (result != STATUS_OK) {
        Serial.println(F("FileSystemManager: Serial plugin init failed"));
    } else {
        Serial.println(F("FileSystemManager: Serial plugin OK"));
    }
    
    // Auto-detect best available storage
    currentStorageType = autoDetectStorage();
    currentStorage = getPluginByType(currentStorageType);
    
    initialized = true;
    
    if (debugEnabled) {
        Serial.print(F("FileSystemManager: Initialized with storage: "));
        if (currentStorage) {
            Serial.println(currentStorage->getName());
        } else {
            Serial.println(F("NONE"));
        }
    }
    
    return STATUS_OK;
}

int FileSystemManager::update() {
    if (!initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    // Check current storage status
    if (currentStorage && !currentStorage->isReady()) {
        // Try to auto-detect alternative storage
        IStoragePlugin::StorageType newType = autoDetectStorage();
        if (newType != currentStorageType) {
            if (debugEnabled) {
                Serial.println(F("FileSystemManager: Storage changed, switching"));
            }
            setStorageType(newType);
        }
    }
    
    return STATUS_OK;
}

int FileSystemManager::getStatus() const {
    if (!initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    if (!currentStorage || !currentStorage->isReady()) {
        return STATUS_ERROR;
    }
    
    return STATUS_OK;
}

const __FlashStringHelper* FileSystemManager::getName() const {
    return F("FileSystemManager");
}

bool FileSystemManager::validate() const {
    return initialized && 
           currentStorage != nullptr &&
           currentStorage->validate();
}

int FileSystemManager::reset() {
    if (initialized) {
        // Reset statistics
        totalFilesWritten = 0;
        totalBytesWritten = 0;
        totalFilesRead = 0;
        totalBytesRead = 0;
        
        // Clear buffers
        clearBuffer(transferBuffer, TRANSFER_BUFFER_SIZE);
        clearBuffer(filenameBuffer, MAX_FILENAME_LENGTH);
        
        initialized = false;
    }
    
    return initialize();
}

size_t FileSystemManager::getMemoryUsage() const {
    size_t totalUsage = sizeof(*this);
    
    if (sdCardPlugin) {
        totalUsage += sdCardPlugin->getMemoryUsage();
    }
    if (eepromPlugin) {
        totalUsage += eepromPlugin->getMemoryUsage();
    }
    if (serialPlugin) {
        totalUsage += serialPlugin->getMemoryUsage();
    }
    
    return totalUsage;
}

void FileSystemManager::setDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

bool FileSystemManager::isDebugEnabled() const {
    return debugEnabled;
}

void FileSystemManager::registerPlugins(SDCardStoragePlugin* sdPlugin, 
                                       EEPROMStoragePlugin* eepromPlugin_,
                                       SerialStoragePlugin* serialPlugin_) {
    sdCardPlugin = sdPlugin;
    eepromPlugin = eepromPlugin_;
    serialPlugin = serialPlugin_;
    
    if (debugEnabled) {
        Serial.println(F("FileSystemManager: Plugins registered"));
    }
}

IStoragePlugin* FileSystemManager::getPluginByType(IStoragePlugin::StorageType type) {
    switch (type) {
        case IStoragePlugin::STORAGE_SD_CARD:
            return sdCardPlugin;
        case IStoragePlugin::STORAGE_EEPROM:
            return eepromPlugin;
        case IStoragePlugin::STORAGE_SERIAL:
            return serialPlugin;
        case IStoragePlugin::STORAGE_AUTO:
        default:
            return nullptr;
    }
}

IStoragePlugin::StorageType FileSystemManager::autoDetectStorage() {
    // Priority order: SD Card > EEPROM > Serial
    if (sdCardPlugin && sdCardPlugin->isReady()) {
        return IStoragePlugin::STORAGE_SD_CARD;
    }
    
    if (eepromPlugin && eepromPlugin->isReady()) {
        return IStoragePlugin::STORAGE_EEPROM;
    }
    
    if (serialPlugin && serialPlugin->isReady()) {
        return IStoragePlugin::STORAGE_SERIAL;
    }
    
    return IStoragePlugin::STORAGE_SD_CARD; // Default fallback
}

bool FileSystemManager::setStorageType(IStoragePlugin::StorageType type) {
    if (!initialized) {
        return false;
    }
    
    IStoragePlugin* newStorage = getPluginByType(type);
    if (!newStorage || !newStorage->isReady()) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Storage type not available"));
        }
        return false;
    }
    
    currentStorage = newStorage;
    currentStorageType = type;
    
    if (debugEnabled) {
        Serial.print(F("FileSystemManager: Storage set to "));
        Serial.println(currentStorage->getName());
    }
    
    return true;
}

IStoragePlugin::StorageType FileSystemManager::getCurrentStorageType() const {
    return currentStorageType;
}

const __FlashStringHelper* FileSystemManager::getCurrentStorageName() const {
    if (currentStorage) {
        return currentStorage->getName();
    }
    return F("NONE");
}

bool FileSystemManager::isStorageReady() const {
    return currentStorage && currentStorage->isReady();
}

bool FileSystemManager::isValidFilename(const char* filename) const {
    if (!filename) {
        return false;
    }
    
    size_t len = safeStrlen(filename, MAX_FILENAME_LENGTH);
    if (len == 0 || len >= MAX_FILENAME_LENGTH) {
        return false;
    }
    
    // Check for invalid characters
    for (size_t i = 0; i < len; i++) {
        char ch = filename[i];
        if (ch < 32 || ch == '/' || ch == '\\' || ch == ':' || 
            ch == '*' || ch == '?' || ch == '"' || ch == '<' || 
            ch == '>' || ch == '|') {
            return false;
        }
    }
    
    return true;
}

bool FileSystemManager::generateUniqueFilename(const char* prefix, const char* extension, 
                                             char* dest, size_t destSize) {
    if (!prefix || !extension || !dest || destSize < 16) {
        return false;
    }
    
    // Get TimeManager for timestamp
    auto timeManager = ServiceLocator::getTimeManager();
    if (!timeManager) {
        // Fallback to simple counter
        static uint16_t counter = 1;
        return snprintf(dest, destSize, "%s_%04d%s", prefix, counter++, extension) > 0;
    }
    
    // Generate timestamp-based filename (will implement when TimeManager is ready)
    static uint16_t counter = 1;
    return snprintf(dest, destSize, "%s_%04d%s", prefix, counter++, extension) > 0;
}

size_t FileSystemManager::writeFile(const char* filename, const uint8_t* data, size_t size) {
    if (!initialized || !currentStorage || !filename || !data || size == 0) {
        return 0;
    }
    
    if (!isValidFilename(filename)) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Invalid filename"));
        }
        return 0;
    }
    
    if (!currentStorage->isReady()) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Storage not ready"));
        }
        return 0;
    }
    
    size_t bytesWritten = currentStorage->writeFile(filename, data, size);
    
    if (bytesWritten > 0) {
        totalFilesWritten++;
        totalBytesWritten += bytesWritten;
        
        if (debugEnabled) {
            Serial.print(F("FileSystemManager: Wrote "));
            Serial.print(bytesWritten);
            Serial.print(F(" bytes to "));
            Serial.println(filename);
        }
    } else if (debugEnabled) {
        Serial.print(F("FileSystemManager: Failed to write "));
        Serial.println(filename);
    }
    
    return bytesWritten;
}

size_t FileSystemManager::writeFileAuto(const char* prefix, const char* extension,
                                       const uint8_t* data, size_t size,
                                       char* generatedName, size_t nameBufferSize) {
    if (!prefix || !extension || !data || size == 0) {
        return 0;
    }
    
    char filename[MAX_FILENAME_LENGTH];
    if (!generateUniqueFilename(prefix, extension, filename, sizeof(filename))) {
        return 0;
    }
    
    // Copy generated name to output buffer if provided
    if (generatedName && nameBufferSize > 0) {
        safeCopy(generatedName, nameBufferSize, filename);
    }
    
    return writeFile(filename, data, size);
}

size_t FileSystemManager::readFile(const char* filename, uint8_t* data, size_t maxSize) {
    if (!initialized || !currentStorage || !filename || !data || maxSize == 0) {
        return 0;
    }
    
    if (!isValidFilename(filename)) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Invalid filename"));
        }
        return 0;
    }
    
    if (!currentStorage->isReady()) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Storage not ready"));
        }
        return 0;
    }
    
    size_t bytesRead = currentStorage->readFile(filename, data, maxSize);
    
    if (bytesRead > 0) {
        totalFilesRead++;
        totalBytesRead += bytesRead;
        
        if (debugEnabled) {
            Serial.print(F("FileSystemManager: Read "));
            Serial.print(bytesRead);
            Serial.print(F(" bytes from "));
            Serial.println(filename);
        }
    } else if (debugEnabled) {
        Serial.print(F("FileSystemManager: Failed to read "));
        Serial.println(filename);
    }
    
    return bytesRead;
}

bool FileSystemManager::copyFile(const char* filename, IStoragePlugin::StorageType sourceType,
                                IStoragePlugin::StorageType destType) {
    if (!filename || sourceType == destType) {
        return false;
    }
    
    IStoragePlugin* sourcePlugin = getPluginByType(sourceType);
    IStoragePlugin* destPlugin = getPluginByType(destType);
    
    if (!sourcePlugin || !destPlugin || 
        !sourcePlugin->isReady() || !destPlugin->isReady()) {
        return false;
    }
    
    // Check if source file exists
    if (!sourcePlugin->fileExists(filename)) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Source file not found"));
        }
        return false;
    }
    
    uint32_t fileSize = sourcePlugin->getFileSize(filename);
    if (fileSize == 0 || fileSize > TRANSFER_BUFFER_SIZE) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: File too large for transfer"));
        }
        return false;
    }
    
    // Read from source
    size_t bytesRead = sourcePlugin->readFile(filename, transferBuffer, fileSize);
    if (bytesRead != fileSize) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Failed to read source file"));
        }
        return false;
    }
    
    // Write to destination
    size_t bytesWritten = destPlugin->writeFile(filename, transferBuffer, bytesRead);
    if (bytesWritten != bytesRead) {
        if (debugEnabled) {
            Serial.println(F("FileSystemManager: Failed to write destination file"));
        }
        return false;
    }
    
    if (debugEnabled) {
        Serial.print(F("FileSystemManager: Copied "));
        Serial.print(filename);
        Serial.print(F(" ("));
        Serial.print(bytesWritten);
        Serial.println(F(" bytes)"));
    }
    
    return true;
}

bool FileSystemManager::deleteFile(const char* filename) {
    if (!initialized || !currentStorage || !filename) {
        return false;
    }
    
    bool result = currentStorage->deleteFile(filename);
    
    if (result && debugEnabled) {
        Serial.print(F("FileSystemManager: Deleted "));
        Serial.println(filename);
    }
    
    return result;
}

bool FileSystemManager::fileExists(const char* filename) const {
    if (!initialized || !currentStorage || !filename) {
        return false;
    }
    
    return currentStorage->fileExists(filename);
}

uint32_t FileSystemManager::getFileSize(const char* filename) const {
    if (!initialized || !currentStorage || !filename) {
        return 0;
    }
    
    return currentStorage->getFileSize(filename);
}

size_t FileSystemManager::listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const {
    if (!initialized || !currentStorage || !filenames || maxFiles == 0) {
        return 0;
    }
    
    return currentStorage->listFiles(filenames, maxFiles);
}

bool FileSystemManager::getStorageSpace(uint32_t& available, uint32_t& total) const {
    if (!initialized || !currentStorage) {
        available = 0;
        total = 0;
        return false;
    }
    
    available = currentStorage->getAvailableSpace();
    total = currentStorage->getTotalSpace();
    return true;
}

void FileSystemManager::getStatistics(uint32_t& filesWritten, uint32_t& bytesWritten,
                                     uint32_t& filesRead, uint32_t& bytesRead) const {
    filesWritten = totalFilesWritten;
    bytesWritten = totalBytesWritten;
    filesRead = totalFilesRead;
    bytesRead = totalBytesRead;
}

bool FileSystemManager::formatStorage() {
    if (!initialized || !currentStorage) {
        return false;
    }
    
    bool result = currentStorage->format();
    
    if (result && debugEnabled) {
        Serial.println(F("FileSystemManager: Storage formatted"));
    }
    
    return result;
}

bool FileSystemManager::getStorageStatus(char* statusBuffer, size_t bufferSize) const {
    if (!initialized || !currentStorage || !statusBuffer || bufferSize == 0) {
        return false;
    }
    
    return currentStorage->getStatus(statusBuffer, bufferSize);
}

bool FileSystemManager::testWrite(const uint8_t* testData, size_t testSize) {
    if (!initialized || !currentStorage) {
        return false;
    }
    
    // Generate test data if not provided
    uint8_t localTestData[32];
    if (!testData) {
        for (size_t i = 0; i < 32; i++) {
            localTestData[i] = (uint8_t)(i + 0xA5);
        }
        testData = localTestData;
        testSize = 32;
    }
    
    // Write test file
    const char* testFilename = "test.dat";
    size_t bytesWritten = writeFile(testFilename, testData, testSize);
    
    if (bytesWritten != testSize) {
        return false;
    }
    
    // Read back and verify
    uint8_t readBuffer[32];
    size_t bytesRead = readFile(testFilename, readBuffer, testSize);
    
    if (bytesRead != testSize) {
        deleteFile(testFilename); // Cleanup
        return false;
    }
    
    // Verify data
    for (size_t i = 0; i < testSize; i++) {
        if (readBuffer[i] != testData[i]) {
            deleteFile(testFilename); // Cleanup
            return false;
        }
    }
    
    // Cleanup
    deleteFile(testFilename);
    
    if (debugEnabled) {
        Serial.println(F("FileSystemManager: Test write successful"));
    }
    
    return true;
}

bool FileSystemManager::validateAllStorages() const {
    bool allValid = true;
    
    if (sdCardPlugin && !sdCardPlugin->validate()) {
        allValid = false;
    }
    
    if (eepromPlugin && !eepromPlugin->validate()) {
        allValid = false;
    }
    
    if (serialPlugin && !serialPlugin->validate()) {
        allValid = false;
    }
    
    return allValid;
}

IStoragePlugin* FileSystemManager::getPlugin(IStoragePlugin::StorageType type) {
    return getPluginByType(type);
}