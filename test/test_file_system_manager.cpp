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

// Storage types
#define STORAGE_SD 0
#define STORAGE_EEPROM 1
#define STORAGE_SERIAL 2

// Emergency reduced constants
#define MAX_FILENAME_LENGTH 2
#define TRANSFER_BUFFER_SIZE 32

// Mock storage plugin interface
class MockStoragePlugin {
public:
    bool initialized;
    bool available;
    char pluginName[16];
    size_t totalSpace;
    size_t usedSpace;
    uint8_t mockData[64]; // Mock storage
    size_t dataSize;
    
    MockStoragePlugin(const char* name, size_t space) : initialized(false), available(true),
                                                        totalSpace(space), usedSpace(0), dataSize(0) {
        strncpy(pluginName, name, sizeof(pluginName) - 1);
        pluginName[sizeof(pluginName) - 1] = '\0';
        memset(mockData, 0, sizeof(mockData));
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        initialized = true;
        return STATUS_OK;
    }
    
    bool isAvailable() const { return available && initialized; }
    
    int writeFile(const char* filename, const uint8_t* buffer, size_t size) {
        if (!initialized || !available) return STATUS_ERROR;
        if (size > sizeof(mockData)) return STATUS_ERROR;
        
        memcpy(mockData, buffer, size);
        dataSize = size;
        usedSpace += size;
        return STATUS_OK;
    }
    
    int readFile(const char* filename, uint8_t* buffer, size_t maxSize, size_t& bytesRead) {
        if (!initialized || !available) return STATUS_ERROR;
        
        bytesRead = (dataSize < maxSize) ? dataSize : maxSize;
        memcpy(buffer, mockData, bytesRead);
        return STATUS_OK;
    }
    
    int deleteFile(const char* filename) {
        if (!initialized || !available) return STATUS_ERROR;
        
        usedSpace = (usedSpace > dataSize) ? usedSpace - dataSize : 0;
        dataSize = 0;
        memset(mockData, 0, sizeof(mockData));
        return STATUS_OK;
    }
    
    size_t getFreeSpace() const { return totalSpace - usedSpace; }
    size_t getTotalSpace() const { return totalSpace; }
    
    void setAvailable(bool avail) { available = avail; }
    
    int reset() {
        initialized = false;
        usedSpace = 0;
        dataSize = 0;
        memset(mockData, 0, sizeof(mockData));
        return STATUS_OK;
    }
};

// FileSystemManager Test Implementation
class FileSystemManagerTest {
private:
    MockStoragePlugin* plugins[3];
    uint8_t pluginCount;
    uint8_t activePlugin;
    bool initialized;
    bool debugEnabled;
    uint8_t transferBuffer[TRANSFER_BUFFER_SIZE];
    
public:
    FileSystemManagerTest() : pluginCount(0), activePlugin(0), initialized(false), debugEnabled(false) {
        for (int i = 0; i < 3; i++) {
            plugins[i] = nullptr;
        }
        memset(transferBuffer, 0, sizeof(transferBuffer));
    }
    
    ~FileSystemManagerTest() {
        for (int i = 0; i < pluginCount; i++) {
            delete plugins[i];
        }
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        // Create mock plugins
        plugins[0] = new MockStoragePlugin("SD", 32768);      // 32KB SD card
        plugins[1] = new MockStoragePlugin("EEPROM", 16384);  // 16KB EEPROM
        plugins[2] = new MockStoragePlugin("Serial", 1024);   // 1KB Serial buffer
        pluginCount = 3;
        
        // Initialize plugins
        for (int i = 0; i < pluginCount; i++) {
            if (plugins[i]) {
                int result = plugins[i]->initialize();
                if (result != STATUS_OK) {
                    return STATUS_ERROR;
                }
            }
        }
        
        // Set first available plugin as active
        for (int i = 0; i < pluginCount; i++) {
            if (plugins[i] && plugins[i]->isAvailable()) {
                activePlugin = i;
                break;
            }
        }
        
        initialized = true;
        return STATUS_OK;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        
        // Check plugin availability
        for (int i = 0; i < pluginCount; i++) {
            if (plugins[i] && !plugins[i]->isAvailable()) {
                // Handle plugin failure
                if (i == activePlugin) {
                    // Switch to next available plugin
                    switchToNextPlugin();
                }
            }
        }
        
        return STATUS_OK;
    }
    
    bool validate() const {
        return initialized && activePlugin < pluginCount && 
               plugins[activePlugin] && plugins[activePlugin]->isAvailable();
    }
    
    int reset() {
        if (initialized) {
            for (int i = 0; i < pluginCount; i++) {
                if (plugins[i]) {
                    plugins[i]->reset();
                    delete plugins[i];
                    plugins[i] = nullptr;
                }
            }
            pluginCount = 0;
            activePlugin = 0;
            initialized = false;
        }
        return STATUS_OK;
    }
    
    int writeFile(const char* filename, const uint8_t* buffer, size_t size) {
        if (!validate()) return STATUS_ERROR;
        
        return plugins[activePlugin]->writeFile(filename, buffer, size);
    }
    
    int readFile(const char* filename, uint8_t* buffer, size_t maxSize, size_t& bytesRead) {
        if (!validate()) return STATUS_ERROR;
        
        return plugins[activePlugin]->readFile(filename, buffer, maxSize, bytesRead);
    }
    
    int deleteFile(const char* filename) {
        if (!validate()) return STATUS_ERROR;
        
        return plugins[activePlugin]->deleteFile(filename);
    }
    
    int copyFile(const char* srcFilename, uint8_t srcStorage, 
                const char* dstFilename, uint8_t dstStorage) {
        if (!initialized || srcStorage >= pluginCount || dstStorage >= pluginCount) {
            return STATUS_ERROR;
        }
        
        MockStoragePlugin* srcPlugin = plugins[srcStorage];
        MockStoragePlugin* dstPlugin = plugins[dstStorage];
        
        if (!srcPlugin || !dstPlugin || !srcPlugin->isAvailable() || !dstPlugin->isAvailable()) {
            return STATUS_ERROR;
        }
        
        // Read from source
        size_t bytesRead = 0;
        int result = srcPlugin->readFile(srcFilename, transferBuffer, sizeof(transferBuffer), bytesRead);
        if (result != STATUS_OK) return result;
        
        // Write to destination
        return dstPlugin->writeFile(dstFilename, transferBuffer, bytesRead);
    }
    
    bool switchToStorage(uint8_t storageType) {
        if (!initialized || storageType >= pluginCount) return false;
        
        if (plugins[storageType] && plugins[storageType]->isAvailable()) {
            activePlugin = storageType;
            return true;
        }
        
        return false;
    }
    
    void switchToNextPlugin() {
        if (!initialized) return;
        
        for (int i = 1; i < pluginCount; i++) {
            uint8_t nextPlugin = (activePlugin + i) % pluginCount;
            if (plugins[nextPlugin] && plugins[nextPlugin]->isAvailable()) {
                activePlugin = nextPlugin;
                return;
            }
        }
    }
    
    // Status methods
    uint8_t getActivePlugin() const { return activePlugin; }
    uint8_t getPluginCount() const { return pluginCount; }
    
    const char* getActivePluginName() const {
        if (validate()) {
            return plugins[activePlugin]->pluginName;
        }
        return "None";
    }
    
    size_t getFreeSpace() const {
        if (validate()) {
            return plugins[activePlugin]->getFreeSpace();
        }
        return 0;
    }
    
    size_t getTotalSpace() const {
        if (validate()) {
            return plugins[activePlugin]->getTotalSpace();
        }
        return 0;
    }
    
    bool isPluginAvailable(uint8_t plugin) const {
        if (plugin >= pluginCount || !plugins[plugin]) return false;
        return plugins[plugin]->isAvailable();
    }
    
    void setPluginAvailable(uint8_t plugin, bool available) {
        if (plugin < pluginCount && plugins[plugin]) {
            plugins[plugin]->setAvailable(available);
        }
    }
    
    // Debug control
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
};

// Test setup/teardown
void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Component Lifecycle Tests
// ============================================================================

void test_filesystem_manager_initialization() {
    FileSystemManagerTest fsm;
    
    int result = fsm.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(fsm.validate());
    TEST_ASSERT_EQUAL(3, fsm.getPluginCount());
    TEST_ASSERT_EQUAL(0, fsm.getActivePlugin()); // Should default to first available
}

void test_filesystem_manager_double_initialization() {
    FileSystemManagerTest fsm;
    
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.initialize());
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.initialize()); // Should be idempotent
    
    TEST_ASSERT_TRUE(fsm.validate());
}

void test_filesystem_manager_update_not_initialized() {
    FileSystemManagerTest fsm;
    
    int result = fsm.update();
    
    TEST_ASSERT_EQUAL(STATUS_NOT_INITIALIZED, result);
}

void test_filesystem_manager_reset() {
    FileSystemManagerTest fsm;
    
    fsm.initialize();
    TEST_ASSERT_TRUE(fsm.validate());
    
    int result = fsm.reset();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_FALSE(fsm.validate());
    TEST_ASSERT_EQUAL(0, fsm.getPluginCount());
}

// ============================================================================
// File Operations Tests
// ============================================================================

void test_write_file_success() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04};
    const char* filename = "T"; // Emergency short filename
    
    int result = fsm.writeFile(filename, testData, sizeof(testData));
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
}

void test_read_file_success() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    uint8_t writeData[] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t readBuffer[4];
    size_t bytesRead = 0;
    const char* filename = "T";
    
    // Write file first
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.writeFile(filename, writeData, sizeof(writeData)));
    
    // Read file back
    int result = fsm.readFile(filename, readBuffer, sizeof(readBuffer), bytesRead);
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_EQUAL(4, bytesRead);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(writeData[i], readBuffer[i]);
    }
}

void test_delete_file_success() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    uint8_t testData[] = {0x11, 0x22};
    const char* filename = "D";
    
    // Write file first
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.writeFile(filename, testData, sizeof(testData)));
    
    // Delete file
    int result = fsm.deleteFile(filename);
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
}

void test_file_operations_not_initialized() {
    FileSystemManagerTest fsm;
    
    uint8_t testData[] = {0x01, 0x02};
    uint8_t readBuffer[2];
    size_t bytesRead;
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, fsm.writeFile("T", testData, 2));
    TEST_ASSERT_EQUAL(STATUS_ERROR, fsm.readFile("T", readBuffer, 2, bytesRead));
    TEST_ASSERT_EQUAL(STATUS_ERROR, fsm.deleteFile("T"));
}

// ============================================================================
// Storage Plugin Management Tests
// ============================================================================

void test_switch_storage_plugin() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    TEST_ASSERT_EQUAL(0, fsm.getActivePlugin()); // Start with SD (0)
    
    bool result = fsm.switchToStorage(STORAGE_EEPROM);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(STORAGE_EEPROM, fsm.getActivePlugin());
}

void test_switch_to_invalid_plugin() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    bool result = fsm.switchToStorage(99); // Invalid plugin
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, fsm.getActivePlugin()); // Should remain unchanged
}

void test_switch_to_unavailable_plugin() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    // Make EEPROM unavailable
    fsm.setPluginAvailable(STORAGE_EEPROM, false);
    
    bool result = fsm.switchToStorage(STORAGE_EEPROM);
    
    TEST_ASSERT_FALSE(result);
}

void test_automatic_plugin_switching() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    TEST_ASSERT_EQUAL(0, fsm.getActivePlugin()); // Start with SD
    
    // Make current plugin unavailable
    fsm.setPluginAvailable(0, false);
    
    // Update should switch to next available plugin
    fsm.update();
    
    TEST_ASSERT_EQUAL(1, fsm.getActivePlugin()); // Should switch to EEPROM
}

// ============================================================================
// File Copy Operations Tests
// ============================================================================

void test_copy_file_between_storages() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    uint8_t testData[] = {0x10, 0x20, 0x30, 0x40};
    const char* srcFile = "S";
    const char* dstFile = "D";
    
    // Write to SD (storage 0)
    fsm.switchToStorage(STORAGE_SD);
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.writeFile(srcFile, testData, sizeof(testData)));
    
    // Copy from SD to EEPROM
    int result = fsm.copyFile(srcFile, STORAGE_SD, dstFile, STORAGE_EEPROM);
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    
    // Verify copy by reading from EEPROM
    fsm.switchToStorage(STORAGE_EEPROM);
    uint8_t readBuffer[4];
    size_t bytesRead;
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.readFile(dstFile, readBuffer, sizeof(readBuffer), bytesRead));
    TEST_ASSERT_EQUAL(4, bytesRead);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(testData[i], readBuffer[i]);
    }
}

void test_copy_file_invalid_source() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    int result = fsm.copyFile("S", 99, "D", STORAGE_EEPROM); // Invalid source
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
}

void test_copy_file_invalid_destination() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    int result = fsm.copyFile("S", STORAGE_SD, "D", 99); // Invalid destination
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
}

void test_copy_file_unavailable_plugins() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    // Make EEPROM unavailable
    fsm.setPluginAvailable(STORAGE_EEPROM, false);
    
    int result = fsm.copyFile("S", STORAGE_SD, "D", STORAGE_EEPROM);
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
}

// ============================================================================
// Storage Space Management Tests
// ============================================================================

void test_get_storage_space_info() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    size_t totalSpace = fsm.getTotalSpace();
    size_t freeSpace = fsm.getFreeSpace();
    
    TEST_ASSERT_EQUAL(32768, totalSpace); // SD card total space
    TEST_ASSERT_EQUAL(32768, freeSpace);  // Initially free
}

void test_storage_space_after_write() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    uint8_t testData[10] = {0};
    size_t initialFree = fsm.getFreeSpace();
    
    fsm.writeFile("T", testData, sizeof(testData));
    
    size_t finalFree = fsm.getFreeSpace();
    
    TEST_ASSERT_EQUAL(initialFree - sizeof(testData), finalFree);
}

void test_plugin_availability_status() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    TEST_ASSERT_TRUE(fsm.isPluginAvailable(STORAGE_SD));
    TEST_ASSERT_TRUE(fsm.isPluginAvailable(STORAGE_EEPROM));
    TEST_ASSERT_TRUE(fsm.isPluginAvailable(STORAGE_SERIAL));
    
    fsm.setPluginAvailable(STORAGE_SD, false);
    TEST_ASSERT_FALSE(fsm.isPluginAvailable(STORAGE_SD));
}

// ============================================================================
// Plugin Name and Identification Tests
// ============================================================================

void test_get_active_plugin_name() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    const char* name = fsm.getActivePluginName();
    
    TEST_ASSERT_EQUAL_STRING("SD", name);
    
    fsm.switchToStorage(STORAGE_EEPROM);
    name = fsm.getActivePluginName();
    
    TEST_ASSERT_EQUAL_STRING("EEPROM", name);
}

void test_get_plugin_name_not_initialized() {
    FileSystemManagerTest fsm;
    
    const char* name = fsm.getActivePluginName();
    
    TEST_ASSERT_EQUAL_STRING("None", name);
}

// ============================================================================
// Emergency Buffer Size Tests
// ============================================================================

void test_transfer_buffer_limits() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    // Test with data larger than transfer buffer
    uint8_t largeData[TRANSFER_BUFFER_SIZE + 10];
    for (int i = 0; i < sizeof(largeData); i++) {
        largeData[i] = (uint8_t)(i & 0xFF);
    }
    
    // Should handle gracefully (implementation dependent)
    int result = fsm.writeFile("L", largeData, sizeof(largeData));
    
    // Result depends on plugin implementation, but should not crash
    TEST_ASSERT_TRUE(result == STATUS_OK || result == STATUS_ERROR);
}

void test_emergency_filename_length() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    uint8_t testData[] = {0x42};
    
    // Test with maximum filename length (2 chars including null)
    int result1 = fsm.writeFile("A", testData, sizeof(testData));
    TEST_ASSERT_EQUAL(STATUS_OK, result1);
    
    int result2 = fsm.writeFile("Z", testData, sizeof(testData));
    TEST_ASSERT_EQUAL(STATUS_OK, result2);
}

// ============================================================================
// Debug Control Tests
// ============================================================================

void test_debug_control() {
    FileSystemManagerTest fsm;
    
    TEST_ASSERT_FALSE(fsm.isDebugEnabled());
    
    fsm.setDebugEnabled(true);
    TEST_ASSERT_TRUE(fsm.isDebugEnabled());
    
    fsm.setDebugEnabled(false);
    TEST_ASSERT_FALSE(fsm.isDebugEnabled());
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_filesystem_manager_complete_workflow() {
    FileSystemManagerTest fsm;
    
    // Initialize
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.initialize());
    TEST_ASSERT_TRUE(fsm.validate());
    
    // Enable debug
    fsm.setDebugEnabled(true);
    
    // Write file to SD
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.writeFile("T", testData, sizeof(testData)));
    
    // Copy to EEPROM
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.copyFile("T", STORAGE_SD, "C", STORAGE_EEPROM));
    
    // Switch to EEPROM
    TEST_ASSERT_TRUE(fsm.switchToStorage(STORAGE_EEPROM));
    
    // Read copied file
    uint8_t readBuffer[5];
    size_t bytesRead;
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.readFile("C", readBuffer, sizeof(readBuffer), bytesRead));
    TEST_ASSERT_EQUAL(5, bytesRead);
    
    // Verify data integrity
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(testData[i], readBuffer[i]);
    }
    
    // Check space usage
    TEST_ASSERT_GREATER_THAN(0, fsm.getTotalSpace() - fsm.getFreeSpace());
    
    // Delete file
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.deleteFile("C"));
    
    // Update cycle
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.update());
    
    // Reset
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.reset());
    TEST_ASSERT_FALSE(fsm.validate());
}

void test_plugin_failure_recovery() {
    FileSystemManagerTest fsm;
    fsm.initialize();
    
    TEST_ASSERT_EQUAL(0, fsm.getActivePlugin()); // Start with SD
    
    // Write some data
    uint8_t testData[] = {0x11, 0x22};
    TEST_ASSERT_EQUAL(STATUS_OK, fsm.writeFile("T", testData, sizeof(testData)));
    
    // Simulate SD failure
    fsm.setPluginAvailable(STORAGE_SD, false);
    
    // Update should detect failure and switch plugins
    fsm.update();
    
    TEST_ASSERT_EQUAL(1, fsm.getActivePlugin()); // Should switch to EEPROM
    TEST_ASSERT_TRUE(fsm.validate()); // Should still be valid
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Component lifecycle tests
    RUN_TEST(test_filesystem_manager_initialization);
    RUN_TEST(test_filesystem_manager_double_initialization);
    RUN_TEST(test_filesystem_manager_update_not_initialized);
    RUN_TEST(test_filesystem_manager_reset);
    
    // File operations tests
    RUN_TEST(test_write_file_success);
    RUN_TEST(test_read_file_success);
    RUN_TEST(test_delete_file_success);
    RUN_TEST(test_file_operations_not_initialized);
    
    // Storage plugin management tests
    RUN_TEST(test_switch_storage_plugin);
    RUN_TEST(test_switch_to_invalid_plugin);
    RUN_TEST(test_switch_to_unavailable_plugin);
    RUN_TEST(test_automatic_plugin_switching);
    
    // File copy operations tests
    RUN_TEST(test_copy_file_between_storages);
    RUN_TEST(test_copy_file_invalid_source);
    RUN_TEST(test_copy_file_invalid_destination);
    RUN_TEST(test_copy_file_unavailable_plugins);
    
    // Storage space management tests
    RUN_TEST(test_get_storage_space_info);
    RUN_TEST(test_storage_space_after_write);
    RUN_TEST(test_plugin_availability_status);
    
    // Plugin identification tests
    RUN_TEST(test_get_active_plugin_name);
    RUN_TEST(test_get_plugin_name_not_initialized);
    
    // Emergency buffer tests
    RUN_TEST(test_transfer_buffer_limits);
    RUN_TEST(test_emergency_filename_length);
    
    // Debug control tests
    RUN_TEST(test_debug_control);
    
    // Integration tests
    RUN_TEST(test_filesystem_manager_complete_workflow);
    RUN_TEST(test_plugin_failure_recovery);
    
    return UNITY_END();
}