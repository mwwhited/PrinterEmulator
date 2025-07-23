#include "SerialStoragePlugin.h"
#include "MemoryUtils.h"

SerialStoragePlugin::SerialStoragePlugin() 
    : initialized(false), debugEnabled(false), transferInProgress(false),
      totalBytesTransferred(0), totalFilesTransferred(0) {
    clearBuffer(currentFilename, sizeof(currentFilename));
    clearBuffer(hexLineBuffer, sizeof(hexLineBuffer));
    clearBuffer(hexByteBuffer, sizeof(hexByteBuffer));
}

int SerialStoragePlugin::initialize() {
    if (initialized) {
        return STATUS_OK;
    }
    
    // Serial should already be initialized by main application
    if (!Serial) {
        Serial.begin(SERIAL_BAUD_RATE);
        delay(100); // Allow serial to stabilize
    }
    
    initialized = true;
    
    if (debugEnabled) {
        Serial.println(F("SerialStoragePlugin: Initialized"));
    }
    
    return STATUS_OK;
}

bool SerialStoragePlugin::isReady() const {
    return initialized && isSerialReady();
}

IStoragePlugin::StorageType SerialStoragePlugin::getType() const {
    return STORAGE_SERIAL;
}

const __FlashStringHelper* SerialStoragePlugin::getName() const {
    return F("Serial");
}

uint32_t SerialStoragePlugin::getAvailableSpace() const {
    // Serial transfer has unlimited space (streaming)
    return 0xFFFFFFFF;
}

uint32_t SerialStoragePlugin::getTotalSpace() const {
    // Serial transfer has unlimited space (streaming)
    return 0xFFFFFFFF;
}

void SerialStoragePlugin::byteToHex(uint8_t value, char* dest) {
    if (!dest) return;
    
    static const char hexChars[] = "0123456789ABCDEF";
    dest[0] = hexChars[(value >> 4) & 0x0F];
    dest[1] = hexChars[value & 0x0F];
    dest[2] = '\0';
}

bool SerialStoragePlugin::hexToByte(const char* hexStr, uint8_t& value) {
    if (!hexStr || safeStrlen(hexStr, 3) < 2) {
        return false;
    }
    
    value = 0;
    
    for (int i = 0; i < 2; i++) {
        char ch = hexStr[i];
        uint8_t nibble;
        
        if (ch >= '0' && ch <= '9') {
            nibble = ch - '0';
        } else if (ch >= 'A' && ch <= 'F') {
            nibble = ch - 'A' + 10;
        } else if (ch >= 'a' && ch <= 'f') {
            nibble = ch - 'a' + 10;
        } else {
            return false;
        }
        
        value = (value << 4) | nibble;
    }
    
    return true;
}

void SerialStoragePlugin::sendProtocolHeader(const char* filename, uint32_t fileSize) {
    Serial.print(F("BEGIN:"));
    Serial.print(filename);
    Serial.print(PROTOCOL_CRLF);
    
    Serial.print(F("SIZE:"));
    Serial.print(fileSize);
    Serial.print(PROTOCOL_CRLF);
    
    flushSerial();
}

void SerialStoragePlugin::sendProtocolFooter(const char* filename) {
    Serial.print(F("END:"));
    Serial.print(filename);
    Serial.print(PROTOCOL_CRLF);
    
    flushSerial();
}

void SerialStoragePlugin::sendHexLine(const uint8_t* data, size_t size, uint32_t address) {
    if (!data || size == 0 || size > HEX_BYTES_PER_LINE) {
        return;
    }
    
    // Clear line buffer
    clearBuffer(hexLineBuffer, sizeof(hexLineBuffer));
    
    // Add address prefix (optional, for debugging)
    if (debugEnabled) {
        char addrStr[10];
        snprintf(addrStr, sizeof(addrStr), "%08X: ", address);
        safeCopy(hexLineBuffer, sizeof(hexLineBuffer), addrStr);
    }
    
    // Convert bytes to hex
    for (size_t i = 0; i < size; i++) {
        byteToHex(data[i], hexByteBuffer);
        appendString(hexLineBuffer, sizeof(hexLineBuffer), hexByteBuffer);
        
        // Add space every 8 bytes for readability
        if ((i + 1) % 8 == 0 && i < size - 1) {
            appendString(hexLineBuffer, sizeof(hexLineBuffer), " ");
        }
    }
    
    // Send line
    Serial.print(hexLineBuffer);
    Serial.print(PROTOCOL_CRLF);
}

void SerialStoragePlugin::flushSerial() {
    Serial.flush();
}

bool SerialStoragePlugin::isSerialReady() const {
    return Serial && Serial.availableForWrite() > 0;
}

size_t SerialStoragePlugin::writeFile(const char* filename, const uint8_t* data, size_t size) {
    if (!initialized || !filename || !data || size == 0) {
        return 0;
    }
    
    return streamFile(filename, data, size);
}

size_t SerialStoragePlugin::streamFile(const char* filename, const uint8_t* data, size_t size) {
    if (!isReady() || !filename || !data || size == 0) {
        return 0;
    }
    
    if (transferInProgress) {
        if (debugEnabled) {
            Serial.println(F("SerialStoragePlugin: Transfer already in progress"));
        }
        return 0;
    }
    
    // Start transfer
    transferInProgress = true;
    safeCopy(currentFilename, sizeof(currentFilename), filename);
    
    // Send protocol header
    sendProtocolHeader(filename, size);
    
    // Send data in hex lines
    const uint8_t* srcData = data;
    size_t remaining = size;
    uint32_t address = 0;
    size_t totalSent = 0;
    
    while (remaining > 0) {
        size_t lineSize = min(remaining, HEX_BYTES_PER_LINE);
        
        sendHexLine(srcData, lineSize, address);
        
        srcData += lineSize;
        remaining -= lineSize;
        address += lineSize;
        totalSent += lineSize;
        
        // Send progress update every 1KB
        if (totalSent % 1024 == 0) {
            sendProgressUpdate(filename, totalSent, size);
        }
        
        // Small delay to prevent overwhelming receiver
        delay(1);
    }
    
    // Send protocol footer
    sendProtocolFooter(filename);
    
    // Update statistics
    totalFilesTransferred++;
    totalBytesTransferred += totalSent;
    
    // End transfer
    transferInProgress = false;
    clearBuffer(currentFilename, sizeof(currentFilename));
    
    if (debugEnabled) {
        Serial.print(F("SerialStoragePlugin: Streamed "));
        Serial.print(totalSent);
        Serial.print(F(" bytes as "));
        Serial.println(filename);
    }
    
    return totalSent;
}

size_t SerialStoragePlugin::readFile(const char* filename, uint8_t* data, size_t maxSize) {
    if (!initialized || !filename || !data || maxSize == 0) {
        return 0;
    }
    
    // Serial storage doesn't support reading - it's write-only (streaming)
    if (debugEnabled) {
        Serial.println(F("SerialStoragePlugin: Read not supported (write-only)"));
    }
    
    return 0;
}

size_t SerialStoragePlugin::receiveFile(uint8_t* data, size_t maxSize, uint32_t timeoutMs) {
    if (!isReady() || !data || maxSize == 0) {
        return 0;
    }
    
    uint32_t startTime = millis();
    size_t bytesReceived = 0;
    char lineBuffer[256];
    size_t linePos = 0;
    
    while (bytesReceived < maxSize && (millis() - startTime) < timeoutMs) {
        if (Serial.available() > 0) {
            char ch = Serial.read();
            
            if (ch == '\r' || ch == '\n') {
                if (linePos > 0) {
                    // Process hex line
                    lineBuffer[linePos] = '\0';
                    
                    // Skip protocol lines and address prefixes
                    const char* hexStart = lineBuffer;
                    if (startsWith(lineBuffer, linePos, "BEGIN:") ||
                        startsWith(lineBuffer, linePos, "END:") ||
                        startsWith(lineBuffer, linePos, "SIZE:")) {
                        linePos = 0;
                        continue;
                    }
                    
                    // Skip address prefix if present
                    const char* colonPos = findChar(lineBuffer, linePos, ':');
                    if (colonPos) {
                        hexStart = colonPos + 2; // Skip ": "
                    }
                    
                    // Convert hex pairs to bytes
                    size_t hexLen = safeStrlen(hexStart, 256);
                    for (size_t i = 0; i < hexLen - 1 && bytesReceived < maxSize; i += 2) {
                        uint8_t byte;
                        if (hexToByte(hexStart + i, byte)) {
                            data[bytesReceived++] = byte;
                        }
                    }
                    
                    linePos = 0;
                }
            } else if (linePos < sizeof(lineBuffer) - 1) {
                lineBuffer[linePos++] = ch;
            }
        } else {
            delay(1);
        }
    }
    
    return bytesReceived;
}

bool SerialStoragePlugin::deleteFile(const char* filename) {
    // Serial storage doesn't support file deletion (streaming only)
    return false;
}

bool SerialStoragePlugin::fileExists(const char* filename) const {
    // Serial storage doesn't maintain file listings
    return false;
}

uint32_t SerialStoragePlugin::getFileSize(const char* filename) const {
    // Serial storage doesn't maintain file information
    return 0;
}

size_t SerialStoragePlugin::listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const {
    // Serial storage doesn't maintain file listings
    return 0;
}

bool SerialStoragePlugin::format() {
    // Reset transfer statistics
    resetTransferStats();
    return true;
}

bool SerialStoragePlugin::getStatus(char* statusBuffer, size_t bufferSize) const {
    if (!statusBuffer || bufferSize == 0) {
        return false;
    }
    
    safeCopy(statusBuffer, bufferSize, "Serial: ");
    
    if (!initialized) {
        appendString(statusBuffer, bufferSize, "Not initialized");
    } else if (!isSerialReady()) {
        appendString(statusBuffer, bufferSize, "Not ready");
    } else if (transferInProgress) {
        appendString(statusBuffer, bufferSize, "Transfer in progress");
    } else {
        appendString(statusBuffer, bufferSize, "Ready");
    }
    
    return true;
}

bool SerialStoragePlugin::validate() const {
    return initialized && isSerialReady();
}

size_t SerialStoragePlugin::getMemoryUsage() const {
    return sizeof(*this);
}

bool SerialStoragePlugin::isTransferInProgress() const {
    return transferInProgress;
}

void SerialStoragePlugin::abortTransfer() {
    if (transferInProgress) {
        transferInProgress = false;
        clearBuffer(currentFilename, sizeof(currentFilename));
        
        // Send abort message
        Serial.println(F("ABORT:Transfer aborted"));
        flushSerial();
        
        if (debugEnabled) {
            Serial.println(F("SerialStoragePlugin: Transfer aborted"));
        }
    }
}

void SerialStoragePlugin::getTransferStats(uint32_t& filesTransferred, uint32_t& bytesTransferred) const {
    filesTransferred = totalFilesTransferred;
    bytesTransferred = totalBytesTransferred;
}

void SerialStoragePlugin::resetTransferStats() {
    totalFilesTransferred = 0;
    totalBytesTransferred = 0;
    
    if (debugEnabled) {
        Serial.println(F("SerialStoragePlugin: Statistics reset"));
    }
}

void SerialStoragePlugin::setDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

void SerialStoragePlugin::sendProgressUpdate(const char* filename, uint32_t bytesTransferred, uint32_t totalBytes) {
    if (!debugEnabled || !filename) {
        return;
    }
    
    uint8_t percentage = (totalBytes > 0) ? (bytesTransferred * 100) / totalBytes : 0;
    
    Serial.print(F("PROGRESS:"));
    Serial.print(filename);
    Serial.print(F(":"));
    Serial.print(bytesTransferred);
    Serial.print(F("/"));
    Serial.print(totalBytes);
    Serial.print(F(" ("));
    Serial.print(percentage);
    Serial.print(F("%)"));
    Serial.print(PROTOCOL_CRLF);
    
    flushSerial();
}

bool SerialStoragePlugin::testProtocol() {
    if (!isReady()) {
        return false;
    }
    
    // Create test data
    uint8_t testData[32];
    for (size_t i = 0; i < 32; i++) {
        testData[i] = (uint8_t)(i + 0xA0);
    }
    
    // Stream test data
    size_t bytesSent = streamFile("test.dat", testData, sizeof(testData));
    
    bool success = (bytesSent == sizeof(testData));
    
    if (debugEnabled) {
        Serial.print(F("SerialStoragePlugin: Protocol test "));
        Serial.println(success ? F("PASSED") : F("FAILED"));
    }
    
    return success;
}

void SerialStoragePlugin::setHexBytesPerLine(size_t bytesPerLine) {
    // This would require modifying the HEX_BYTES_PER_LINE constant
    // For now, just log the request
    if (debugEnabled) {
        Serial.print(F("SerialStoragePlugin: Hex bytes per line change requested: "));
        Serial.println(bytesPerLine);
    }
}