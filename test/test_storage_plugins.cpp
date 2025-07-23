#include <unity.h>
#include <stdint.h>
#include <string.h>

// Mock Arduino functions
#define F(x) x

// Status codes
#define STATUS_OK 0
#define STATUS_ERROR 1
#define STATUS_NOT_INITIALIZED 2
#define STATUS_BUSY 3

// Emergency reduced constants
#define MAX_FILENAME_LENGTH 2
#define TRANSFER_BUFFER_SIZE 32
#define EEPROM_BLOCK_SIZE 32

// Mock SPI and SD library functions
static bool mock_sd_begin_success = true;
static bool mock_sd_available = true;

bool mock_SD_begin(uint8_t csPin) {
    return mock_sd_begin_success;
}

// Mock EEPROM SPI functions
static uint8_t mock_eeprom_data[1024] = {0};
static bool mock_eeprom_available = true;

void mock_eeprom_write(uint16_t address, const uint8_t* data, size_t length) {
    if (address + length <= sizeof(mock_eeprom_data)) {
        memcpy(mock_eeprom_data + address, data, length);
    }
}

void mock_eeprom_read(uint16_t address, uint8_t* data, size_t length) {
    if (address + length <= sizeof(mock_eeprom_data)) {
        memcpy(data, mock_eeprom_data + address, length);
    }
}

// Mock Serial functions
static char mock_serial_buffer[256] = {0};
static size_t mock_serial_pos = 0;

void mock_serial_print(const char* str) {
    size_t len = strlen(str);
    if (mock_serial_pos + len < sizeof(mock_serial_buffer)) {
        strncpy(mock_serial_buffer + mock_serial_pos, str, len);
        mock_serial_pos += len;
    }
}

void mock_serial_println(const char* str) {
    mock_serial_print(str);
    mock_serial_print("\n");
}

// ============================================================================
// SD Card Storage Plugin Tests
// ============================================================================

class SDCardStoragePluginTest {
private:
    bool initialized;
    bool available;
    uint8_t csPin;
    char mockFiles[4][MAX_FILENAME_LENGTH + 1];
    uint8_t mockFileData[4][64];
    size_t mockFileSizes[4];
    uint8_t fileCount;
    
public:
    SDCardStoragePluginTest(uint8_t cs = 10) : initialized(false), available(true), 
                                              csPin(cs), fileCount(0) {
        memset(mockFiles, 0, sizeof(mockFiles));
        memset(mockFileData, 0, sizeof(mockFileData));
        memset(mockFileSizes, 0, sizeof(mockFileSizes));
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        if (!mock_SD_begin(csPin)) {
            available = false;
            return STATUS_ERROR;
        }
        
        initialized = true;
        available = mock_sd_available;
        return STATUS_OK;
    }
    
    bool isAvailable() const {
        return initialized && available && mock_sd_available;
    }
    
    int writeFile(const char* filename, const uint8_t* buffer, size_t size) {
        if (!isAvailable()) return STATUS_ERROR;
        if (size > 64) return STATUS_ERROR; // Mock size limit
        
        // Find existing file or create new one
        int fileIndex = findFile(filename);
        if (fileIndex == -1) {
            if (fileCount >= 4) return STATUS_ERROR; // Mock file limit
            fileIndex = fileCount++;
            strncpy(mockFiles[fileIndex], filename, MAX_FILENAME_LENGTH);
        }
        
        memcpy(mockFileData[fileIndex], buffer, size);
        mockFileSizes[fileIndex] = size;
        
        return STATUS_OK;
    }
    
    int readFile(const char* filename, uint8_t* buffer, size_t maxSize, size_t& bytesRead) {
        if (!isAvailable()) return STATUS_ERROR;
        
        int fileIndex = findFile(filename);
        if (fileIndex == -1) return STATUS_ERROR;
        
        bytesRead = (mockFileSizes[fileIndex] < maxSize) ? mockFileSizes[fileIndex] : maxSize;
        memcpy(buffer, mockFileData[fileIndex], bytesRead);
        
        return STATUS_OK;
    }
    
    int deleteFile(const char* filename) {
        if (!isAvailable()) return STATUS_ERROR;
        
        int fileIndex = findFile(filename);
        if (fileIndex == -1) return STATUS_ERROR;
        
        // Shift files down
        for (int i = fileIndex; i < fileCount - 1; i++) {
            strcpy(mockFiles[i], mockFiles[i + 1]);
            memcpy(mockFileData[i], mockFileData[i + 1], 64);
            mockFileSizes[i] = mockFileSizes[i + 1];
        }
        fileCount--;
        
        return STATUS_OK;
    }
    
    size_t getFreeSpace() const {
        return isAvailable() ? (32768 - (fileCount * 100)) : 0; // Mock calculation
    }
    
    size_t getTotalSpace() const {
        return isAvailable() ? 32768 : 0;
    }
    
    int reset() {
        initialized = false;
        available = false;
        fileCount = 0;
        memset(mockFiles, 0, sizeof(mockFiles));
        memset(mockFileData, 0, sizeof(mockFileData));
        memset(mockFileSizes, 0, sizeof(mockFileSizes));
        return STATUS_OK;
    }
    
    void setAvailable(bool avail) { available = avail; }
    uint8_t getFileCount() const { return fileCount; }
    
private:
    int findFile(const char* filename) {
        for (int i = 0; i < fileCount; i++) {
            if (strcmp(mockFiles[i], filename) == 0) {
                return i;
            }
        }
        return -1;
    }
};

// ============================================================================
// EEPROM Storage Plugin Tests
// ============================================================================

class EEPROMStoragePluginTest {
private:
    bool initialized;
    bool available;
    uint16_t baseAddress;
    uint16_t nextAddress;
    uint16_t totalSize;
    
    struct FileEntry {
        char filename[MAX_FILENAME_LENGTH + 1];
        uint16_t address;
        uint16_t size;
        bool valid;
    };
    
    FileEntry fileTable[8]; // Mock file table
    uint8_t fileCount;
    
public:
    EEPROMStoragePluginTest(uint16_t base = 0) : initialized(false), available(true),
                                                baseAddress(base), nextAddress(base + 64),
                                                totalSize(1024), fileCount(0) {
        memset(fileTable, 0, sizeof(fileTable));
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        if (!mock_eeprom_available) {
            available = false;
            return STATUS_ERROR;
        }
        
        // Initialize file system
        nextAddress = baseAddress + 64; // Reserve space for file table
        initialized = true;
        available = true;
        
        return STATUS_OK;
    }
    
    bool isAvailable() const {
        return initialized && available && mock_eeprom_available;
    }
    
    int writeFile(const char* filename, const uint8_t* buffer, size_t size) {
        if (!isAvailable()) return STATUS_ERROR;
        if (size > 256) return STATUS_ERROR; // Mock size limit
        
        // Find existing file or create new one
        int fileIndex = findFile(filename);
        if (fileIndex == -1) {
            if (fileCount >= 8) return STATUS_ERROR; // Mock file limit
            fileIndex = fileCount++;
            strncpy(fileTable[fileIndex].filename, filename, MAX_FILENAME_LENGTH);
            fileTable[fileIndex].address = nextAddress;
            nextAddress += size;
        }
        
        fileTable[fileIndex].size = size;
        fileTable[fileIndex].valid = true;
        
        // Write to mock EEPROM
        mock_eeprom_write(fileTable[fileIndex].address, buffer, size);
        
        return STATUS_OK;
    }
    
    int readFile(const char* filename, uint8_t* buffer, size_t maxSize, size_t& bytesRead) {
        if (!isAvailable()) return STATUS_ERROR;
        
        int fileIndex = findFile(filename);
        if (fileIndex == -1 || !fileTable[fileIndex].valid) return STATUS_ERROR;
        
        bytesRead = (fileTable[fileIndex].size < maxSize) ? fileTable[fileIndex].size : maxSize;
        mock_eeprom_read(fileTable[fileIndex].address, buffer, bytesRead);
        
        return STATUS_OK;
    }
    
    int deleteFile(const char* filename) {
        if (!isAvailable()) return STATUS_ERROR;
        
        int fileIndex = findFile(filename);
        if (fileIndex == -1) return STATUS_ERROR;
        
        fileTable[fileIndex].valid = false;
        
        // Compact file table
        for (int i = fileIndex; i < fileCount - 1; i++) {
            fileTable[i] = fileTable[i + 1];
        }
        fileCount--;
        
        return STATUS_OK;
    }
    
    size_t getFreeSpace() const {
        return isAvailable() ? (totalSize - (nextAddress - baseAddress)) : 0;
    }
    
    size_t getTotalSpace() const {
        return isAvailable() ? totalSize : 0;
    }
    
    int reset() {
        initialized = false;
        available = false;
        fileCount = 0;
        nextAddress = baseAddress + 64;
        memset(fileTable, 0, sizeof(fileTable));
        memset(mock_eeprom_data, 0, sizeof(mock_eeprom_data));
        return STATUS_OK;
    }
    
    void setAvailable(bool avail) { available = avail; }
    uint8_t getFileCount() const { return fileCount; }
    
private:
    int findFile(const char* filename) {
        for (int i = 0; i < fileCount; i++) {
            if (fileTable[i].valid && strcmp(fileTable[i].filename, filename) == 0) {
                return i;
            }
        }
        return -1;
    }
};

// ============================================================================
// Serial Storage Plugin Tests
// ============================================================================

class SerialStoragePluginTest {
private:
    bool initialized;
    bool available;
    uint8_t buffer[128];
    size_t bufferSize;
    char currentFilename[MAX_FILENAME_LENGTH + 1];
    
public:
    SerialStoragePluginTest() : initialized(false), available(true), bufferSize(0) {
        memset(buffer, 0, sizeof(buffer));
        memset(currentFilename, 0, sizeof(currentFilename));
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        initialized = true;
        available = true;
        
        return STATUS_OK;
    }
    
    bool isAvailable() const {
        return initialized && available;
    }
    
    int writeFile(const char* filename, const uint8_t* data, size_t size) {
        if (!isAvailable()) return STATUS_ERROR;
        if (size > sizeof(buffer)) return STATUS_ERROR;
        
        strncpy(currentFilename, filename, MAX_FILENAME_LENGTH);
        memcpy(buffer, data, size);
        bufferSize = size;
        
        // Mock serial output
        mock_serial_println("BEGIN");
        mock_serial_print("FILE:");
        mock_serial_println(filename);
        
        // Output hex data
        for (size_t i = 0; i < size; i++) {
            char hexByte[4];
            snprintf(hexByte, sizeof(hexByte), "%02X", data[i]);
            mock_serial_print(hexByte);
            if ((i + 1) % 16 == 0) mock_serial_print("\n");
        }
        if (size % 16 != 0) mock_serial_print("\n");
        
        mock_serial_println("END");
        
        return STATUS_OK;
    }
    
    int readFile(const char* filename, uint8_t* readBuffer, size_t maxSize, size_t& bytesRead) {
        if (!isAvailable()) return STATUS_ERROR;
        
        if (strcmp(currentFilename, filename) != 0) {
            return STATUS_ERROR; // File not found
        }
        
        bytesRead = (bufferSize < maxSize) ? bufferSize : maxSize;
        memcpy(readBuffer, buffer, bytesRead);
        
        return STATUS_OK;
    }
    
    int deleteFile(const char* filename) {
        if (!isAvailable()) return STATUS_ERROR;
        
        if (strcmp(currentFilename, filename) == 0) {
            memset(currentFilename, 0, sizeof(currentFilename));
            memset(buffer, 0, sizeof(buffer));
            bufferSize = 0;
            return STATUS_OK;
        }
        
        return STATUS_ERROR;
    }
    
    size_t getFreeSpace() const {
        return isAvailable() ? (sizeof(buffer) - bufferSize) : 0;
    }
    
    size_t getTotalSpace() const {
        return isAvailable() ? sizeof(buffer) : 0;
    }
    
    int reset() {
        initialized = false;
        available = false;
        bufferSize = 0;
        memset(buffer, 0, sizeof(buffer));
        memset(currentFilename, 0, sizeof(currentFilename));
        return STATUS_OK;
    }
    
    void setAvailable(bool avail) { available = avail; }
    const char* getCurrentFilename() const { return currentFilename; }
    size_t getBufferSize() const { return bufferSize; }
    const char* getSerialOutput() const { return mock_serial_buffer; }
};

// Test setup/teardown
void setUp(void) {
    mock_sd_begin_success = true;
    mock_sd_available = true;
    mock_eeprom_available = true;
    memset(mock_eeprom_data, 0, sizeof(mock_eeprom_data));
    memset(mock_serial_buffer, 0, sizeof(mock_serial_buffer));
    mock_serial_pos = 0;
}

void tearDown(void) {}

// ============================================================================
// SD Card Plugin Tests
// ============================================================================

void test_sd_card_initialization_success() {
    SDCardStoragePluginTest sd;
    
    int result = sd.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(sd.isAvailable());
}

void test_sd_card_initialization_failure() {
    mock_sd_begin_success = false;
    SDCardStoragePluginTest sd;
    
    int result = sd.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
    TEST_ASSERT_FALSE(sd.isAvailable());
}

void test_sd_card_write_read_file() {
    SDCardStoragePluginTest sd;
    sd.initialize();
    
    uint8_t writeData[] = {0x01, 0x02, 0x03, 0x04};
    const char* filename = "T";
    
    // Write file
    TEST_ASSERT_EQUAL(STATUS_OK, sd.writeFile(filename, writeData, sizeof(writeData)));
    TEST_ASSERT_EQUAL(1, sd.getFileCount());
    
    // Read file back
    uint8_t readBuffer[4];
    size_t bytesRead;
    TEST_ASSERT_EQUAL(STATUS_OK, sd.readFile(filename, readBuffer, sizeof(readBuffer), bytesRead));
    TEST_ASSERT_EQUAL(4, bytesRead);
    
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(writeData[i], readBuffer[i]);
    }
}

void test_sd_card_delete_file() {
    SDCardStoragePluginTest sd;
    sd.initialize();
    
    uint8_t testData[] = {0xAA, 0xBB};
    sd.writeFile("D", testData, sizeof(testData));
    TEST_ASSERT_EQUAL(1, sd.getFileCount());
    
    TEST_ASSERT_EQUAL(STATUS_OK, sd.deleteFile("D"));
    TEST_ASSERT_EQUAL(0, sd.getFileCount());
}

void test_sd_card_space_management() {
    SDCardStoragePluginTest sd;
    sd.initialize();
    
    size_t initialFree = sd.getFreeSpace();
    size_t totalSpace = sd.getTotalSpace();
    
    TEST_ASSERT_EQUAL(32768, totalSpace);
    TEST_ASSERT_GREATER_THAN(0, initialFree);
}

// ============================================================================
// EEPROM Plugin Tests
// ============================================================================

void test_eeprom_initialization_success() {
    EEPROMStoragePluginTest eeprom;
    
    int result = eeprom.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(eeprom.isAvailable());
}

void test_eeprom_initialization_failure() {
    mock_eeprom_available = false;
    EEPROMStoragePluginTest eeprom;
    
    int result = eeprom.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
    TEST_ASSERT_FALSE(eeprom.isAvailable());
}

void test_eeprom_write_read_file() {
    EEPROMStoragePluginTest eeprom;
    eeprom.initialize();
    
    uint8_t writeData[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    const char* filename = "E";
    
    // Write file
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.writeFile(filename, writeData, sizeof(writeData)));
    TEST_ASSERT_EQUAL(1, eeprom.getFileCount());
    
    // Read file back
    uint8_t readBuffer[5];
    size_t bytesRead;
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.readFile(filename, readBuffer, sizeof(readBuffer), bytesRead));
    TEST_ASSERT_EQUAL(5, bytesRead);
    
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(writeData[i], readBuffer[i]);
    }
}

void test_eeprom_delete_file() {
    EEPROMStoragePluginTest eeprom;
    eeprom.initialize();
    
    uint8_t testData[] = {0x11, 0x22, 0x33};
    eeprom.writeFile("D", testData, sizeof(testData));
    TEST_ASSERT_EQUAL(1, eeprom.getFileCount());
    
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.deleteFile("D"));
    TEST_ASSERT_EQUAL(0, eeprom.getFileCount());
}

void test_eeprom_space_management() {
    EEPROMStoragePluginTest eeprom;
    eeprom.initialize();
    
    size_t initialFree = eeprom.getFreeSpace();
    size_t totalSpace = eeprom.getTotalSpace();
    
    TEST_ASSERT_EQUAL(1024, totalSpace);
    TEST_ASSERT_GREATER_THAN(900, initialFree); // Should have most space free initially
}

void test_eeprom_multiple_files() {
    EEPROMStoragePluginTest eeprom;
    eeprom.initialize();
    
    uint8_t data1[] = {0x01};
    uint8_t data2[] = {0x02};
    
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.writeFile("1", data1, sizeof(data1)));
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.writeFile("2", data2, sizeof(data2)));
    TEST_ASSERT_EQUAL(2, eeprom.getFileCount());
    
    uint8_t readBuffer[1];
    size_t bytesRead;
    
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.readFile("2", readBuffer, 1, bytesRead));
    TEST_ASSERT_EQUAL(0x02, readBuffer[0]);
}

// ============================================================================
// Serial Plugin Tests
// ============================================================================

void test_serial_initialization() {
    SerialStoragePluginTest serial;
    
    int result = serial.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(serial.isAvailable());
}

void test_serial_write_file() {
    SerialStoragePluginTest serial;
    serial.initialize();
    
    uint8_t testData[] = {0xAB, 0xCD, 0xEF};
    const char* filename = "S";
    
    int result = serial.writeFile(filename, testData, sizeof(testData));
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_EQUAL_STRING("S", serial.getCurrentFilename());
    TEST_ASSERT_EQUAL(3, serial.getBufferSize());
}

void test_serial_read_file() {
    SerialStoragePluginTest serial;
    serial.initialize();
    
    uint8_t writeData[] = {0x12, 0x34};
    serial.writeFile("R", writeData, sizeof(writeData));
    
    uint8_t readBuffer[2];
    size_t bytesRead;
    int result = serial.readFile("R", readBuffer, sizeof(readBuffer), bytesRead);
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_EQUAL(2, bytesRead);
    TEST_ASSERT_EQUAL(0x12, readBuffer[0]);
    TEST_ASSERT_EQUAL(0x34, readBuffer[1]);
}

void test_serial_delete_file() {
    SerialStoragePluginTest serial;
    serial.initialize();
    
    uint8_t testData[] = {0xFF};
    serial.writeFile("D", testData, sizeof(testData));
    
    TEST_ASSERT_EQUAL(STATUS_OK, serial.deleteFile("D"));
    TEST_ASSERT_EQUAL_STRING("", serial.getCurrentFilename());
    TEST_ASSERT_EQUAL(0, serial.getBufferSize());
}

void test_serial_hex_output_format() {
    SerialStoragePluginTest serial;
    serial.initialize();
    
    uint8_t testData[] = {0xAB, 0xCD};
    serial.writeFile("H", testData, sizeof(testData));
    
    const char* output = serial.getSerialOutput();
    
    // Should contain BEGIN, filename, hex data, and END
    TEST_ASSERT_NOT_NULL(strstr(output, "BEGIN"));
    TEST_ASSERT_NOT_NULL(strstr(output, "FILE:H"));
    TEST_ASSERT_NOT_NULL(strstr(output, "ABCD"));
    TEST_ASSERT_NOT_NULL(strstr(output, "END"));
}

// ============================================================================
// Plugin Unavailability Tests
// ============================================================================

void test_sd_card_unavailable_operations() {
    SDCardStoragePluginTest sd;
    sd.initialize();
    sd.setAvailable(false);
    
    uint8_t testData[] = {0x01};
    uint8_t readBuffer[1];
    size_t bytesRead;
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, sd.writeFile("T", testData, 1));
    TEST_ASSERT_EQUAL(STATUS_ERROR, sd.readFile("T", readBuffer, 1, bytesRead));
    TEST_ASSERT_EQUAL(STATUS_ERROR, sd.deleteFile("T"));
}

void test_eeprom_unavailable_operations() {
    EEPROMStoragePluginTest eeprom;
    eeprom.initialize();
    eeprom.setAvailable(false);
    
    uint8_t testData[] = {0x01};
    uint8_t readBuffer[1];
    size_t bytesRead;
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, eeprom.writeFile("T", testData, 1));
    TEST_ASSERT_EQUAL(STATUS_ERROR, eeprom.readFile("T", readBuffer, 1, bytesRead));
    TEST_ASSERT_EQUAL(STATUS_ERROR, eeprom.deleteFile("T"));
}

void test_serial_unavailable_operations() {
    SerialStoragePluginTest serial;
    serial.initialize();
    serial.setAvailable(false);
    
    uint8_t testData[] = {0x01};
    uint8_t readBuffer[1];
    size_t bytesRead;
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, serial.writeFile("T", testData, 1));
    TEST_ASSERT_EQUAL(STATUS_ERROR, serial.readFile("T", readBuffer, 1, bytesRead));
    TEST_ASSERT_EQUAL(STATUS_ERROR, serial.deleteFile("T"));
}

// ============================================================================
// Emergency Buffer Size Tests
// ============================================================================

void test_plugins_emergency_filename_limits() {
    SDCardStoragePluginTest sd;
    EEPROMStoragePluginTest eeprom;
    SerialStoragePluginTest serial;
    
    sd.initialize();
    eeprom.initialize();
    serial.initialize();
    
    uint8_t testData[] = {0x42};
    
    // Test maximum filename length (1 char + null terminator)
    TEST_ASSERT_EQUAL(STATUS_OK, sd.writeFile("A", testData, 1));
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.writeFile("B", testData, 1));
    TEST_ASSERT_EQUAL(STATUS_OK, serial.writeFile("C", testData, 1));
}

void test_plugins_large_data_handling() {
    SDCardStoragePluginTest sd;
    EEPROMStoragePluginTest eeprom;
    SerialStoragePluginTest serial;
    
    sd.initialize();
    eeprom.initialize();
    serial.initialize();
    
    // Test with data at buffer limits
    uint8_t largeData[64];
    for (int i = 0; i < 64; i++) {
        largeData[i] = (uint8_t)(i & 0xFF);
    }
    
    // SD should handle 64 bytes
    TEST_ASSERT_EQUAL(STATUS_OK, sd.writeFile("L", largeData, 64));
    
    // EEPROM should handle large data
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.writeFile("L", largeData, 64));
    
    // Serial should handle up to buffer size
    TEST_ASSERT_EQUAL(STATUS_OK, serial.writeFile("L", largeData, 64));
}

// ============================================================================
// Plugin Reset Tests
// ============================================================================

void test_sd_card_reset() {
    SDCardStoragePluginTest sd;
    sd.initialize();
    
    uint8_t testData[] = {0x01};
    sd.writeFile("T", testData, 1);
    TEST_ASSERT_EQUAL(1, sd.getFileCount());
    
    TEST_ASSERT_EQUAL(STATUS_OK, sd.reset());
    TEST_ASSERT_FALSE(sd.isAvailable());
    TEST_ASSERT_EQUAL(0, sd.getFileCount());
}

void test_eeprom_reset() {
    EEPROMStoragePluginTest eeprom;
    eeprom.initialize();
    
    uint8_t testData[] = {0x01};
    eeprom.writeFile("T", testData, 1);
    TEST_ASSERT_EQUAL(1, eeprom.getFileCount());
    
    TEST_ASSERT_EQUAL(STATUS_OK, eeprom.reset());
    TEST_ASSERT_FALSE(eeprom.isAvailable());
    TEST_ASSERT_EQUAL(0, eeprom.getFileCount());
}

void test_serial_reset() {
    SerialStoragePluginTest serial;
    serial.initialize();
    
    uint8_t testData[] = {0x01};
    serial.writeFile("T", testData, 1);
    TEST_ASSERT_EQUAL(1, serial.getBufferSize());
    
    TEST_ASSERT_EQUAL(STATUS_OK, serial.reset());
    TEST_ASSERT_FALSE(serial.isAvailable());
    TEST_ASSERT_EQUAL(0, serial.getBufferSize());
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // SD Card plugin tests
    RUN_TEST(test_sd_card_initialization_success);
    RUN_TEST(test_sd_card_initialization_failure);
    RUN_TEST(test_sd_card_write_read_file);
    RUN_TEST(test_sd_card_delete_file);
    RUN_TEST(test_sd_card_space_management);
    
    // EEPROM plugin tests
    RUN_TEST(test_eeprom_initialization_success);
    RUN_TEST(test_eeprom_initialization_failure);
    RUN_TEST(test_eeprom_write_read_file);
    RUN_TEST(test_eeprom_delete_file);
    RUN_TEST(test_eeprom_space_management);
    RUN_TEST(test_eeprom_multiple_files);
    
    // Serial plugin tests
    RUN_TEST(test_serial_initialization);
    RUN_TEST(test_serial_write_file);
    RUN_TEST(test_serial_read_file);
    RUN_TEST(test_serial_delete_file);
    RUN_TEST(test_serial_hex_output_format);
    
    // Plugin unavailability tests
    RUN_TEST(test_sd_card_unavailable_operations);
    RUN_TEST(test_eeprom_unavailable_operations);
    RUN_TEST(test_serial_unavailable_operations);
    
    // Emergency buffer tests
    RUN_TEST(test_plugins_emergency_filename_limits);
    RUN_TEST(test_plugins_large_data_handling);
    
    // Plugin reset tests
    RUN_TEST(test_sd_card_reset);
    RUN_TEST(test_eeprom_reset);
    RUN_TEST(test_serial_reset);
    
    return UNITY_END();
}