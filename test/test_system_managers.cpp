#include <unity.h>
#include <stdint.h>
#include <string.h>

// Mock Arduino functions
#define F(x) x
#define millis() mock_millis()
#define digitalWrite(pin, val) mock_digitalWrite(pin, val)

// Mock hardware state
static uint32_t mock_time = 0;
static bool mock_pin_states[50] = {false};

uint32_t mock_millis() {
    return mock_time;
}

void mock_digitalWrite(uint8_t pin, uint8_t val) {
    mock_pin_states[pin] = (val == 1);
}

// Status codes
#define STATUS_OK 0
#define STATUS_ERROR 1
#define STATUS_NOT_INITIALIZED 2

// Hardware pins
#define HEARTBEAT_LED_PIN 13

// ============================================================================
// HeartbeatLEDManager Tests
// ============================================================================

class HeartbeatLEDManagerTest {
private:
    bool initialized;
    bool debugEnabled;
    uint32_t lastBlinkTime;
    bool ledState;
    uint16_t blinkInterval;
    uint8_t errorPattern;
    uint8_t patternIndex;
    
    // SOS pattern: dot dot dot dash dash dash dot dot dot
    static const uint16_t SOS_PATTERN[9];
    static const uint8_t SOS_PATTERN_LENGTH = 9;
    
public:
    HeartbeatLEDManagerTest() : initialized(false), debugEnabled(false),
                               lastBlinkTime(0), ledState(false),
                               blinkInterval(1000), errorPattern(0), patternIndex(0) {}
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        // Setup LED pin
        // pinMode(HEARTBEAT_LED_PIN, OUTPUT); // Mock
        digitalWrite(HEARTBEAT_LED_PIN, 0); // LED off
        
        initialized = true;
        return STATUS_OK;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        
        uint32_t currentTime = millis();
        
        if (errorPattern > 0) {
            updateErrorPattern(currentTime);
        } else {
            updateNormalHeartbeat(currentTime);
        }
        
        return STATUS_OK;
    }
    
    bool validate() const {
        return initialized;
    }
    
    int reset() {
        if (initialized) {
            digitalWrite(HEARTBEAT_LED_PIN, 0);
            initialized = false;
            errorPattern = 0;
            patternIndex = 0;
            ledState = false;
        }
        return STATUS_OK;
    }
    
    void setErrorPattern(uint8_t pattern) {
        errorPattern = pattern;
        patternIndex = 0;
        lastBlinkTime = millis();
    }
    
    void clearErrorPattern() {
        errorPattern = 0;
        patternIndex = 0;
        blinkInterval = 1000; // Reset to normal heartbeat
    }
    
    void setBlinkInterval(uint16_t interval) {
        blinkInterval = interval;
    }
    
    // Status methods
    bool getLEDState() const { return ledState; }
    uint8_t getErrorPattern() const { return errorPattern; }
    uint16_t getBlinkInterval() const { return blinkInterval; }
    uint8_t getPatternIndex() const { return patternIndex; }
    
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
    
private:
    void updateNormalHeartbeat(uint32_t currentTime) {
        if (currentTime - lastBlinkTime >= blinkInterval) {
            ledState = !ledState;
            digitalWrite(HEARTBEAT_LED_PIN, ledState ? 1 : 0);
            lastBlinkTime = currentTime;
        }
    }
    
    void updateErrorPattern(uint32_t currentTime) {
        if (errorPattern == 1) { // SOS pattern
            updateSOSPattern(currentTime);
        } else {
            // Other error patterns can be added here
            updateNormalHeartbeat(currentTime);
        }
    }
    
    void updateSOSPattern(uint32_t currentTime) {
        uint16_t interval = SOS_PATTERN[patternIndex];
        
        if (currentTime - lastBlinkTime >= interval) {
            ledState = !ledState;
            digitalWrite(HEARTBEAT_LED_PIN, ledState ? 1 : 0);
            lastBlinkTime = currentTime;
            
            if (!ledState) { // LED just turned off, advance pattern
                patternIndex++;
                if (patternIndex >= SOS_PATTERN_LENGTH) {
                    patternIndex = 0;
                    // Add pause between SOS repetitions
                    lastBlinkTime += 2000;
                }
            }
        }
    }
};

// SOS pattern timing (in milliseconds)
const uint16_t HeartbeatLEDManagerTest::SOS_PATTERN[9] = {
    200, 200,   // S: dot
    200, 200,   // S: dot  
    200, 600,   // S: dot + pause
    600, 200,   // O: dash
    600, 200,   // O: dash
    600, 600,   // O: dash + pause
    200, 200,   // S: dot
    200, 200,   // S: dot
    200, 1000   // S: dot + long pause
};

// ============================================================================
// ConfigurationManager Tests
// ============================================================================

class ConfigurationManagerTest {
private:
    bool initialized;
    bool debugEnabled;
    
    // Emergency minimal configuration
    struct Config {
        uint8_t activeStorage;
        uint16_t captureInterval;
        bool debugMode;
        char deviceName[4]; // Emergency short name
    } config;
    
public:
    ConfigurationManagerTest() : initialized(false), debugEnabled(false) {
        // Default configuration
        config.activeStorage = 0;
        config.captureInterval = 1000;
        config.debugMode = false;
        strncpy(config.deviceName, "Meg", 4);
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        // Load default configuration
        initialized = true;
        return STATUS_OK;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        return STATUS_OK;
    }
    
    bool validate() const {
        return initialized;
    }
    
    int reset() {
        if (initialized) {
            initialized = false;
        }
        return STATUS_OK;
    }
    
    // Configuration accessors
    uint8_t getActiveStorage() const { return config.activeStorage; }
    void setActiveStorage(uint8_t storage) { config.activeStorage = storage; }
    
    uint16_t getCaptureInterval() const { return config.captureInterval; }
    void setCaptureInterval(uint16_t interval) { config.captureInterval = interval; }
    
    bool getDebugMode() const { return config.debugMode; }
    void setDebugMode(bool enabled) { config.debugMode = enabled; }
    
    const char* getDeviceName() const { return config.deviceName; }
    void setDeviceName(const char* name) {
        strncpy(config.deviceName, name, sizeof(config.deviceName) - 1);
        config.deviceName[sizeof(config.deviceName) - 1] = '\0';
    }
    
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
};

// ============================================================================
// SystemManager Tests
// ============================================================================

class SystemManagerTest {
private:
    bool initialized;
    bool debugEnabled;
    uint32_t lastHealthCheck;
    uint32_t uptime;
    uint16_t systemErrors;
    uint8_t systemStatus;
    
    // System health metrics
    uint16_t freeRAM;
    uint16_t loopCount;
    uint32_t lastLoopTime;
    
public:
    SystemManagerTest() : initialized(false), debugEnabled(false),
                         lastHealthCheck(0), uptime(0), systemErrors(0),
                         systemStatus(STATUS_OK), freeRAM(1024),
                         loopCount(0), lastLoopTime(0) {}
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        uptime = millis();
        systemStatus = STATUS_OK;
        systemErrors = 0;
        
        initialized = true;
        return STATUS_OK;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        
        uint32_t currentTime = millis();
        
        // Update loop performance metrics
        if (lastLoopTime > 0) {
            uint32_t loopDuration = currentTime - lastLoopTime;
            if (loopDuration > 10) { // If loop takes more than 10ms
                systemErrors++;
            }
        }
        lastLoopTime = currentTime;
        loopCount++;
        
        // Periodic health check
        if (currentTime - lastHealthCheck >= 5000) { // Every 5 seconds
            performHealthCheck();
            lastHealthCheck = currentTime;
        }
        
        return STATUS_OK;
    }
    
    bool validate() const {
        return initialized && systemStatus == STATUS_OK;
    }
    
    int reset() {
        if (initialized) {
            initialized = false;
            systemStatus = STATUS_NOT_INITIALIZED;
            systemErrors = 0;
            loopCount = 0;
        }
        return STATUS_OK;
    }
    
    // System status methods
    uint8_t getSystemStatus() const { return systemStatus; }
    uint16_t getSystemErrors() const { return systemErrors; }
    uint32_t getUptime() const { return initialized ? (millis() - uptime) : 0; }
    uint16_t getFreeRAM() const { return freeRAM; }
    uint16_t getLoopCount() const { return loopCount; }
    
    void reportError(const char* errorMsg) {
        systemErrors++;
        if (systemErrors > 10) {
            systemStatus = STATUS_ERROR;
        }
    }
    
    void clearErrors() {
        systemErrors = 0;
        systemStatus = STATUS_OK;
    }
    
    float getLoopFrequency() const {
        if (getUptime() == 0) return 0.0f;
        return (float)loopCount / (getUptime() / 1000.0f);
    }
    
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
    
private:
    void performHealthCheck() {
        // Mock RAM check
        freeRAM = 1195; // Mock current free RAM
        
        // Check system health
        if (freeRAM < 500) {
            reportError("Low RAM");
        }
        
        if (getLoopFrequency() < 100) { // Below 100 loops/sec
            reportError("Low Performance");
        }
    }
};

// ============================================================================
// TimeManager Tests
// ============================================================================

class TimeManagerTest {
private:
    bool initialized;
    bool debugEnabled;
    bool rtcAvailable;
    uint32_t bootTime;
    
    // Mock RTC time
    struct DateTime {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    } currentTime;
    
public:
    TimeManagerTest() : initialized(false), debugEnabled(false),
                       rtcAvailable(true), bootTime(0) {
        // Default time
        currentTime.year = 2025;
        currentTime.month = 1;
        currentTime.day = 23;
        currentTime.hour = 12;
        currentTime.minute = 0;
        currentTime.second = 0;
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        bootTime = millis();
        
        // Mock RTC initialization
        if (!rtcAvailable) {
            return STATUS_ERROR;
        }
        
        initialized = true;
        return STATUS_OK;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        
        // Update time based on millis() if RTC not available
        if (!rtcAvailable) {
            uint32_t elapsed = millis() - bootTime;
            currentTime.second = (currentTime.second + elapsed / 1000) % 60;
        }
        
        return STATUS_OK;
    }
    
    bool validate() const {
        return initialized;
    }
    
    int reset() {
        if (initialized) {
            initialized = false;
        }
        return STATUS_OK;
    }
    
    // Time accessors
    uint16_t getYear() const { return currentTime.year; }
    uint8_t getMonth() const { return currentTime.month; }
    uint8_t getDay() const { return currentTime.day; }
    uint8_t getHour() const { return currentTime.hour; }
    uint8_t getMinute() const { return currentTime.minute; }
    uint8_t getSecond() const { return currentTime.second; }
    
    void setDateTime(uint16_t year, uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second) {
        currentTime.year = year;
        currentTime.month = month;
        currentTime.day = day;
        currentTime.hour = hour;
        currentTime.minute = minute;
        currentTime.second = second;
    }
    
    bool isRTCAvailable() const { return rtcAvailable; }
    void setRTCAvailable(bool available) { rtcAvailable = available; }
    
    uint32_t getUnixTime() const {
        // Simplified Unix timestamp calculation
        return 1737715200 + (currentTime.hour * 3600) + (currentTime.minute * 60) + currentTime.second;
    }
    
    void formatTime(char* buffer, size_t bufferSize) const {
        snprintf(buffer, bufferSize, "%02d:%02d:%02d",
                 currentTime.hour, currentTime.minute, currentTime.second);
    }
    
    void formatDate(char* buffer, size_t bufferSize) const {
        snprintf(buffer, bufferSize, "%04d-%02d-%02d",
                 currentTime.year, currentTime.month, currentTime.day);
    }
    
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
};

// Test setup/teardown
void setUp(void) {
    mock_time = 0;
    memset(mock_pin_states, 0, sizeof(mock_pin_states));
}

void tearDown(void) {}

// ============================================================================
// HeartbeatLEDManager Tests
// ============================================================================

void test_heartbeat_led_initialization() {
    HeartbeatLEDManagerTest hb;
    
    int result = hb.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(hb.validate());
    TEST_ASSERT_FALSE(hb.getLEDState());
}

void test_heartbeat_led_normal_blink() {
    HeartbeatLEDManagerTest hb;
    hb.initialize();
    
    bool initialState = hb.getLEDState();
    
    // Advance time past blink interval
    mock_time = 1500;
    hb.update();
    
    TEST_ASSERT_NOT_EQUAL(initialState, hb.getLEDState());
}

void test_heartbeat_led_sos_pattern() {
    HeartbeatLEDManagerTest hb;
    hb.initialize();
    
    hb.setErrorPattern(1); // SOS pattern
    TEST_ASSERT_EQUAL(1, hb.getErrorPattern());
    
    // Test pattern progression
    for (int i = 0; i < 10; i++) {
        mock_time += 300;
        hb.update();
    }
    
    TEST_ASSERT_GREATER_THAN(0, hb.getPatternIndex());
}

void test_heartbeat_led_clear_error() {
    HeartbeatLEDManagerTest hb;
    hb.initialize();
    
    hb.setErrorPattern(1);
    TEST_ASSERT_EQUAL(1, hb.getErrorPattern());
    
    hb.clearErrorPattern();
    TEST_ASSERT_EQUAL(0, hb.getErrorPattern());
    TEST_ASSERT_EQUAL(1000, hb.getBlinkInterval());
}

void test_heartbeat_led_custom_interval() {
    HeartbeatLEDManagerTest hb;
    hb.initialize();
    
    hb.setBlinkInterval(500);
    TEST_ASSERT_EQUAL(500, hb.getBlinkInterval());
}

// ============================================================================
// ConfigurationManager Tests
// ============================================================================

void test_configuration_manager_initialization() {
    ConfigurationManagerTest config;
    
    int result = config.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(config.validate());
}

void test_configuration_manager_default_values() {
    ConfigurationManagerTest config;
    config.initialize();
    
    TEST_ASSERT_EQUAL(0, config.getActiveStorage());
    TEST_ASSERT_EQUAL(1000, config.getCaptureInterval());
    TEST_ASSERT_FALSE(config.getDebugMode());
    TEST_ASSERT_EQUAL_STRING("Meg", config.getDeviceName());
}

void test_configuration_manager_set_values() {
    ConfigurationManagerTest config;
    config.initialize();
    
    config.setActiveStorage(1);
    config.setCaptureInterval(2000);
    config.setDebugMode(true);
    config.setDeviceName("Dev");
    
    TEST_ASSERT_EQUAL(1, config.getActiveStorage());
    TEST_ASSERT_EQUAL(2000, config.getCaptureInterval());
    TEST_ASSERT_TRUE(config.getDebugMode());
    TEST_ASSERT_EQUAL_STRING("Dev", config.getDeviceName());
}

void test_configuration_manager_device_name_truncation() {
    ConfigurationManagerTest config;
    config.initialize();
    
    config.setDeviceName("VeryLongDeviceName");
    
    // Should be truncated to fit emergency buffer
    const char* name = config.getDeviceName();
    TEST_ASSERT_LESS_THAN(4, strlen(name));
}

// ============================================================================
// SystemManager Tests
// ============================================================================

void test_system_manager_initialization() {
    SystemManagerTest sys;
    
    int result = sys.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(sys.validate());
    TEST_ASSERT_EQUAL(STATUS_OK, sys.getSystemStatus());
    TEST_ASSERT_EQUAL(0, sys.getSystemErrors());
}

void test_system_manager_uptime_tracking() {
    SystemManagerTest sys;
    
    mock_time = 1000;
    sys.initialize();
    
    mock_time = 6000; // 5 seconds later
    sys.update();
    
    uint32_t uptime = sys.getUptime();
    TEST_ASSERT_EQUAL(5000, uptime);
}

void test_system_manager_loop_counting() {
    SystemManagerTest sys;
    sys.initialize();
    
    for (int i = 0; i < 10; i++) {
        mock_time += 1;
        sys.update();
    }
    
    TEST_ASSERT_EQUAL(10, sys.getLoopCount());
}

void test_system_manager_error_reporting() {
    SystemManagerTest sys;
    sys.initialize();
    
    TEST_ASSERT_EQUAL(0, sys.getSystemErrors());
    
    sys.reportError("Test Error");
    TEST_ASSERT_EQUAL(1, sys.getSystemErrors());
    TEST_ASSERT_EQUAL(STATUS_OK, sys.getSystemStatus()); // Still OK with 1 error
    
    // Report many errors
    for (int i = 0; i < 15; i++) {
        sys.reportError("Multiple Errors");
    }
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, sys.getSystemStatus());
}

void test_system_manager_clear_errors() {
    SystemManagerTest sys;
    sys.initialize();
    
    sys.reportError("Test Error");
    TEST_ASSERT_GREATER_THAN(0, sys.getSystemErrors());
    
    sys.clearErrors();
    TEST_ASSERT_EQUAL(0, sys.getSystemErrors());
    TEST_ASSERT_EQUAL(STATUS_OK, sys.getSystemStatus());
}

void test_system_manager_loop_frequency() {
    SystemManagerTest sys;
    sys.initialize();
    
    // Simulate 1 second of operation with 500 loops
    for (int i = 0; i < 500; i++) {
        mock_time += 2; // 2ms per loop
        sys.update();
    }
    
    float frequency = sys.getLoopFrequency();
    TEST_ASSERT_GREATER_THAN(400.0f, frequency); // Should be around 500 Hz
}

// ============================================================================
// TimeManager Tests
// ============================================================================

void test_time_manager_initialization() {
    TimeManagerTest time;
    
    int result = time.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(time.validate());
    TEST_ASSERT_TRUE(time.isRTCAvailable());
}

void test_time_manager_initialization_failure() {
    TimeManagerTest time;
    time.setRTCAvailable(false);
    
    int result = time.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
}

void test_time_manager_default_time() {
    TimeManagerTest time;
    time.initialize();
    
    TEST_ASSERT_EQUAL(2025, time.getYear());
    TEST_ASSERT_EQUAL(1, time.getMonth());
    TEST_ASSERT_EQUAL(23, time.getDay());
    TEST_ASSERT_EQUAL(12, time.getHour());
    TEST_ASSERT_EQUAL(0, time.getMinute());
    TEST_ASSERT_EQUAL(0, time.getSecond());
}

void test_time_manager_set_datetime() {
    TimeManagerTest time;
    time.initialize();
    
    time.setDateTime(2025, 12, 31, 23, 59, 58);
    
    TEST_ASSERT_EQUAL(2025, time.getYear());
    TEST_ASSERT_EQUAL(12, time.getMonth());
    TEST_ASSERT_EQUAL(31, time.getDay());
    TEST_ASSERT_EQUAL(23, time.getHour());
    TEST_ASSERT_EQUAL(59, time.getMinute());
    TEST_ASSERT_EQUAL(58, time.getSecond());
}

void test_time_manager_format_time() {
    TimeManagerTest time;
    time.initialize();
    time.setDateTime(2025, 1, 23, 14, 30, 45);
    
    char timeBuffer[16];
    time.formatTime(timeBuffer, sizeof(timeBuffer));
    
    TEST_ASSERT_EQUAL_STRING("14:30:45", timeBuffer);
}

void test_time_manager_format_date() {
    TimeManagerTest time;
    time.initialize();
    time.setDateTime(2025, 1, 23, 14, 30, 45);
    
    char dateBuffer[16];
    time.formatDate(dateBuffer, sizeof(dateBuffer));
    
    TEST_ASSERT_EQUAL_STRING("2025-01-23", dateBuffer);
}

void test_time_manager_unix_timestamp() {
    TimeManagerTest time;
    time.initialize();
    time.setDateTime(2025, 1, 23, 0, 0, 0);
    
    uint32_t timestamp = time.getUnixTime();
    
    TEST_ASSERT_GREATER_THAN(1737715200, timestamp); // Should be reasonable Unix time
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_all_managers_lifecycle() {
    HeartbeatLEDManagerTest hb;
    ConfigurationManagerTest config;
    SystemManagerTest sys;
    TimeManagerTest time;
    
    // Initialize all
    TEST_ASSERT_EQUAL(STATUS_OK, hb.initialize());
    TEST_ASSERT_EQUAL(STATUS_OK, config.initialize());
    TEST_ASSERT_EQUAL(STATUS_OK, sys.initialize());
    TEST_ASSERT_EQUAL(STATUS_OK, time.initialize());
    
    // Update all
    mock_time += 100;
    TEST_ASSERT_EQUAL(STATUS_OK, hb.update());
    TEST_ASSERT_EQUAL(STATUS_OK, config.update());
    TEST_ASSERT_EQUAL(STATUS_OK, sys.update());
    TEST_ASSERT_EQUAL(STATUS_OK, time.update());
    
    // All should be valid
    TEST_ASSERT_TRUE(hb.validate());
    TEST_ASSERT_TRUE(config.validate());
    TEST_ASSERT_TRUE(sys.validate());
    TEST_ASSERT_TRUE(time.validate());
    
    // Reset all
    TEST_ASSERT_EQUAL(STATUS_OK, hb.reset());
    TEST_ASSERT_EQUAL(STATUS_OK, config.reset());
    TEST_ASSERT_EQUAL(STATUS_OK, sys.reset());
    TEST_ASSERT_EQUAL(STATUS_OK, time.reset());
}

void test_error_condition_handling() {
    HeartbeatLEDManagerTest hb;
    SystemManagerTest sys;
    
    hb.initialize();
    sys.initialize();
    
    // Report system error
    sys.reportError("Critical Error");
    
    // Heartbeat should show error pattern
    hb.setErrorPattern(1); // SOS
    
    // Update both
    for (int i = 0; i < 5; i++) {
        mock_time += 200;
        hb.update();
        sys.update();
    }
    
    TEST_ASSERT_EQUAL(1, hb.getErrorPattern());
    TEST_ASSERT_GREATER_THAN(0, sys.getSystemErrors());
}

void test_debug_enable_all_managers() {
    HeartbeatLEDManagerTest hb;
    ConfigurationManagerTest config;
    SystemManagerTest sys;
    TimeManagerTest time;
    
    hb.initialize();
    config.initialize();
    sys.initialize();
    time.initialize();
    
    // Enable debug on all
    hb.setDebugEnabled(true);
    config.setDebugEnabled(true);
    sys.setDebugEnabled(true);
    time.setDebugEnabled(true);
    
    TEST_ASSERT_TRUE(hb.isDebugEnabled());
    TEST_ASSERT_TRUE(config.isDebugEnabled());
    TEST_ASSERT_TRUE(sys.isDebugEnabled());
    TEST_ASSERT_TRUE(time.isDebugEnabled());
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // HeartbeatLEDManager tests
    RUN_TEST(test_heartbeat_led_initialization);
    RUN_TEST(test_heartbeat_led_normal_blink);
    RUN_TEST(test_heartbeat_led_sos_pattern);
    RUN_TEST(test_heartbeat_led_clear_error);
    RUN_TEST(test_heartbeat_led_custom_interval);
    
    // ConfigurationManager tests
    RUN_TEST(test_configuration_manager_initialization);
    RUN_TEST(test_configuration_manager_default_values);
    RUN_TEST(test_configuration_manager_set_values);
    RUN_TEST(test_configuration_manager_device_name_truncation);
    
    // SystemManager tests
    RUN_TEST(test_system_manager_initialization);
    RUN_TEST(test_system_manager_uptime_tracking);
    RUN_TEST(test_system_manager_loop_counting);
    RUN_TEST(test_system_manager_error_reporting);
    RUN_TEST(test_system_manager_clear_errors);
    RUN_TEST(test_system_manager_loop_frequency);
    
    // TimeManager tests
    RUN_TEST(test_time_manager_initialization);
    RUN_TEST(test_time_manager_initialization_failure);
    RUN_TEST(test_time_manager_default_time);
    RUN_TEST(test_time_manager_set_datetime);
    RUN_TEST(test_time_manager_format_time);
    RUN_TEST(test_time_manager_format_date);
    RUN_TEST(test_time_manager_unix_timestamp);
    
    // Integration tests
    RUN_TEST(test_all_managers_lifecycle);
    RUN_TEST(test_error_condition_handling);
    RUN_TEST(test_debug_enable_all_managers);
    
    return UNITY_END();
}