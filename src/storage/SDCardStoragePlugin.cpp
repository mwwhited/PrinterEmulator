#include "SDCardStoragePlugin.h"
#include "MemoryUtils.h"

SDCardStoragePlugin::SDCardStoragePlugin() 
    : initialized(false), cardPresent(false), writeProtected(false),
      debugEnabled(false), cardSize(0), freeSpace(0) {
    clearBuffer(pathBuffer, sizeof(pathBuffer));
}

int SDCardStoragePlugin::initialize() {
    if (initialized) {
        return STATUS_OK;
    }
    
    // Check initial card status
    checkCardStatus();
    
    if (!cardPresent) {
        if (debugEnabled) {
            Serial.println(F("SDCardStoragePlugin: No card detected"));
        }
        // Don't return error - card might be inserted later
    }
    
    // Initialize SD library
    if (cardPresent && !SD.begin(SD_CS_PIN)) {
        if (debugEnabled) {
            Serial.println(F("SDCardStoragePlugin: SD.begin() failed"));
        }
        cardPresent = false;
    }
    
    if (cardPresent) {
        // Get card information
        File root = SD.open("/");
        if (root) {
            cardSize = root.size(); // This may not work reliably on all cards
            root.close();
        }
        
        updateFreeSpace();
        
        if (debugEnabled) {
            Serial.print(F("SDCardStoragePlugin: Card initialized, size: "));
            Serial.print(cardSize / 1024);
            Serial.println(F("KB"));
        }
    }
    
    initialized = true;
    return STATUS_OK;
}

bool SDCardStoragePlugin::isReady() const {
    return initialized && cardPresent && !writeProtected;
}

IStoragePlugin::StorageType SDCardStoragePlugin::getType() const {
    return STORAGE_SD_CARD;
}

const __FlashStringHelper* SDCardStoragePlugin::getName() const {
    return F("SD Card");
}

uint32_t SDCardStoragePlugin::getAvailableSpace() const {
    return freeSpace;
}

uint32_t SDCardStoragePlugin::getTotalSpace() const {
    return cardSize;
}

void SDCardStoragePlugin::checkCardStatus() {
    // Check card detect pin
    cardPresent = (digitalRead(SD_DETECT_PIN) == LOW); // Active LOW
    
    // Check write protect pin
    writeProtected = (digitalRead(SD_WRITE_PROTECT_PIN) == HIGH); // Active HIGH
    
    if (debugEnabled && cardPresent) {
        Serial.print(F("SDCardStoragePlugin: Card present"));
        if (writeProtected) {
            Serial.println(F(" (write protected)"));
        } else {
            Serial.println();
        }
    }
}

void SDCardStoragePlugin::updateFreeSpace() {
    if (!cardPresent) {
        freeSpace = 0;
        return;
    }
    
    // Note: Getting free space on SD cards is not straightforward with Arduino SD library
    // This is a simplified approach - actual implementation would need card-specific commands
    freeSpace = cardSize / 2; // Rough estimate - assume half free
}

bool SDCardStoragePlugin::ensureDirectoryExists(const char* filename) {
    if (!filename) {
        return false;
    }
    
    // Find last slash to separate directory from filename
    const char* lastSlash = findChar(filename, MAX_FILENAME_LENGTH, '/');
    if (!lastSlash) {
        return true; // No directory component
    }
    
    // Extract directory path
    size_t dirLen = lastSlash - filename;
    if (dirLen >= sizeof(pathBuffer)) {
        return false;
    }
    
    safeCopy(pathBuffer, sizeof(pathBuffer), filename, dirLen);
    
    // Check if directory exists, create if not
    if (!SD.exists(pathBuffer)) {
        return SD.mkdir(pathBuffer);
    }
    
    return true;
}

size_t SDCardStoragePlugin::writeFile(const char* filename, const uint8_t* data, size_t size) {
    if (!isReady() || !filename || !data || size == 0) {
        return 0;
    }
    
    // Refresh card status
    checkCardStatus();
    if (!cardPresent || writeProtected) {
        if (debugEnabled) {
            Serial.println(F("SDCardStoragePlugin: Card not ready for write"));
        }
        return 0;
    }
    
    // Ensure directory structure exists
    if (!ensureDirectoryExists(filename)) {
        if (debugEnabled) {
            Serial.println(F("SDCardStoragePlugin: Failed to create directory"));
        }
        return 0;
    }
    
    // Open file for writing
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        if (debugEnabled) {
            Serial.print(F("SDCardStoragePlugin: Failed to open file: "));
            Serial.println(filename);
        }
        return 0;
    }
    
    // Write data
    size_t bytesWritten = file.write(data, size);
    file.close();
    
    if (bytesWritten != size && debugEnabled) {
        Serial.print(F("SDCardStoragePlugin: Partial write: "));
        Serial.print(bytesWritten);
        Serial.print(F("/"));
        Serial.println(size);
    }
    
    // Update free space estimate
    if (bytesWritten > 0) {
        freeSpace = (freeSpace > bytesWritten) ? freeSpace - bytesWritten : 0;
    }
    
    return bytesWritten;
}

size_t SDCardStoragePlugin::readFile(const char* filename, uint8_t* data, size_t maxSize) {
    if (!initialized || !cardPresent || !filename || !data || maxSize == 0) {
        return 0;
    }
    
    // Refresh card status
    checkCardStatus();
    if (!cardPresent) {
        if (debugEnabled) {
            Serial.println(F("SDCardStoragePlugin: Card not present"));
        }
        return 0;
    }
    
    // Open file for reading
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        if (debugEnabled) {
            Serial.print(F("SDCardStoragePlugin: Failed to open file: "));
            Serial.println(filename);
        }
        return 0;
    }
    
    // Read data
    size_t bytesRead = file.read(data, maxSize);
    file.close();
    
    return bytesRead;
}

bool SDCardStoragePlugin::deleteFile(const char* filename) {
    if (!isReady() || !filename) {
        return false;
    }
    
    // Refresh card status
    checkCardStatus();
    if (!cardPresent || writeProtected) {
        return false;
    }
    
    bool result = SD.remove(filename);
    
    if (debugEnabled) {
        Serial.print(F("SDCardStoragePlugin: Delete "));
        Serial.print(filename);
        Serial.println(result ? F(" - SUCCESS") : F(" - FAILED"));
    }
    
    return result;
}

bool SDCardStoragePlugin::fileExists(const char* filename) const {
    if (!initialized || !cardPresent || !filename) {
        return false;
    }
    
    return SD.exists(filename);
}

uint32_t SDCardStoragePlugin::getFileSize(const char* filename) const {
    if (!initialized || !cardPresent || !filename) {
        return 0;
    }
    
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        return 0;
    }
    
    uint32_t size = file.size();
    file.close();
    
    return size;
}

size_t SDCardStoragePlugin::listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const {
    return listFilesInDirectory(nullptr, filenames, maxFiles);
}

size_t SDCardStoragePlugin::listFilesInDirectory(const char* dirPath, 
                                                char filenames[][MAX_FILENAME_LENGTH], 
                                                size_t maxFiles) const {
    if (!initialized || !cardPresent || !filenames || maxFiles == 0) {
        return 0;
    }
    
    const char* path = dirPath ? dirPath : "/";
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }
    
    size_t fileCount = 0;
    
    while (fileCount < maxFiles) {
        File entry = dir.openNextFile();
        if (!entry) {
            break; // No more files
        }
        
        // Skip directories for now
        if (!entry.isDirectory()) {
            const char* name = entry.name();
            if (name && safeStrlen(name, MAX_FILENAME_LENGTH) < MAX_FILENAME_LENGTH) {
                safeCopy(filenames[fileCount], MAX_FILENAME_LENGTH, name);
                fileCount++;
            }
        }
        
        entry.close();
    }
    
    dir.close();
    return fileCount;
}

bool SDCardStoragePlugin::format() {
    if (!isReady()) {
        return false;
    }
    
    if (debugEnabled) {
        Serial.println(F("SDCardStoragePlugin: Format not supported by Arduino SD library"));
    }
    
    // Arduino SD library doesn't support formatting
    return false;
}

bool SDCardStoragePlugin::getStatus(char* statusBuffer, size_t bufferSize) const {
    if (!statusBuffer || bufferSize == 0) {
        return false;
    }
    
    safeCopy(statusBuffer, bufferSize, "SD: ");
    
    if (!cardPresent) {
        appendString(statusBuffer, bufferSize, "No card");
    } else if (writeProtected) {
        appendString(statusBuffer, bufferSize, "Write protected");
    } else if (!initialized) {
        appendString(statusBuffer, bufferSize, "Not initialized");
    } else {
        appendString(statusBuffer, bufferSize, "Ready");
        
        // Add space information if available
        if (cardSize > 0) {
            char spaceInfo[32];
            snprintf(spaceInfo, sizeof(spaceInfo), " (%luKB)", cardSize / 1024);
            appendString(statusBuffer, bufferSize, spaceInfo);
        }
    }
    
    return true;
}

bool SDCardStoragePlugin::validate() const {
    return initialized && cardPresent;
}

size_t SDCardStoragePlugin::getMemoryUsage() const {
    return sizeof(*this);
}

bool SDCardStoragePlugin::isCardPresent() const {
    return cardPresent;
}

bool SDCardStoragePlugin::isWriteProtected() const {
    return writeProtected;
}

bool SDCardStoragePlugin::getCardType(char* typeBuffer, size_t bufferSize) const {
    if (!typeBuffer || bufferSize == 0 || !cardPresent) {
        return false;
    }
    
    // Arduino SD library doesn't provide easy access to card type
    // This would require low-level SD commands
    safeCopy(typeBuffer, bufferSize, "SDHC/SDXC");
    return true;
}

void SDCardStoragePlugin::refreshCardStatus() {
    checkCardStatus();
    
    if (cardPresent && initialized) {
        updateFreeSpace();
    }
}

void SDCardStoragePlugin::setDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

bool SDCardStoragePlugin::createDirectory(const char* dirPath) {
    if (!isReady() || !dirPath) {
        return false;
    }
    
    return SD.mkdir(dirPath);
}