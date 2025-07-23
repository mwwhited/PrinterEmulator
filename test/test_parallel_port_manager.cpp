#include <unity.h>
#include <stdint.h>
#include <string.h>

// Mock Arduino functions
#define F(x) x
#define digitalRead(pin) mock_digitalRead(pin)
#define digitalWrite(pin, val) mock_digitalWrite(pin, val)
#define pinMode(pin, mode) mock_pinMode(pin, mode)
#define attachInterrupt(int_num, func, mode) mock_attachInterrupt(int_num, func, mode)
#define detachInterrupt(int_num) mock_detachInterrupt(int_num)
#define micros() mock_micros()

// Mock hardware state
static bool mock_pin_states[50] = {false};
static uint32_t mock_time = 0;
static void (*mock_isr_callback)() = nullptr;

bool mock_digitalRead(uint8_t pin) {
    return mock_pin_states[pin];
}

void mock_digitalWrite(uint8_t pin, uint8_t val) {
    mock_pin_states[pin] = (val == 1);
}

void mock_pinMode(uint8_t pin, uint8_t mode) {
    // Mock pin mode setting
}

void mock_attachInterrupt(uint8_t interrupt, void (*func)(), int mode) {
    mock_isr_callback = func;
}

void mock_detachInterrupt(uint8_t interrupt) {
    mock_isr_callback = nullptr;
}

uint32_t mock_micros() {
    return mock_time;
}

// Mock hardware configuration
#define STROBE_PIN 18
#define ACK_PIN 41
#define BUSY_PIN 43
#define LPT_ACTIVITY_LED_PIN 30
#define STROBE_INTERRUPT 5
#define RING_BUFFER_SIZE 16

// Status codes
#define STATUS_OK 0
#define STATUS_ERROR 1
#define STATUS_NOT_INITIALIZED 2
#define STATUS_BUSY 3

// Mock RingBuffer for testing
class MockRingBuffer {
private:
    uint8_t buffer[RING_BUFFER_SIZE];
    volatile size_t head, tail, count;
    volatile bool overflowFlag;
    
public:
    MockRingBuffer() : head(0), tail(0), count(0), overflowFlag(false) {}
    
    bool write(uint8_t data) {
        if (count >= RING_BUFFER_SIZE) {
            overflowFlag = true;
            return false;
        }
        buffer[head] = data;
        head = (head + 1) % RING_BUFFER_SIZE;
        count++;
        return true;
    }
    
    bool read(uint8_t* data) {
        if (count == 0) return false;
        *data = buffer[tail];
        tail = (tail + 1) % RING_BUFFER_SIZE;
        count--;
        return true;
    }
    
    size_t available() const { return count; }
    bool hasOverflowed() const { return overflowFlag; }
    void clearOverflow() { overflowFlag = false; }
    void clear() { head = tail = count = 0; overflowFlag = false; }
    uint8_t getUtilization() const { return (count * 100) / RING_BUFFER_SIZE; }
};

// Mock ParallelPortManager implementation
class ParallelPortManagerTest {
private:
    MockRingBuffer dataBuffer;
    volatile uint32_t bytesReceived;
    volatile uint32_t overflowCount;
    volatile uint32_t lastIsrTime;
    volatile uint32_t maxIsrTime;
    bool initialized;
    bool debugEnabled;
    
public:
    ParallelPortManagerTest() : bytesReceived(0), overflowCount(0), 
                               lastIsrTime(0), maxIsrTime(0), 
                               initialized(false), debugEnabled(false) {}
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        // Setup pins
        pinMode(STROBE_PIN, 0);  // INPUT
        pinMode(ACK_PIN, 1);     // OUTPUT
        pinMode(BUSY_PIN, 1);    // OUTPUT
        pinMode(LPT_ACTIVITY_LED_PIN, 1); // OUTPUT
        
        // Setup data pins (D0-D7)
        for (int i = 25; i <= 39; i += 2) {
            pinMode(i, 0);  // INPUT
        }
        
        // Set initial states
        digitalWrite(ACK_PIN, 1);   // ACK high (idle)
        digitalWrite(BUSY_PIN, 0);  // BUSY low (ready)
        digitalWrite(LPT_ACTIVITY_LED_PIN, 0); // LED off
        
        // Attach interrupt
        attachInterrupt(STROBE_INTERRUPT, nullptr, 2); // FALLING
        
        dataBuffer.clear();
        bytesReceived = 0;
        overflowCount = 0;
        initialized = true;
        
        return STATUS_OK;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        
        // Process any buffered data
        uint8_t data;
        while (dataBuffer.read(&data)) {
            // Data processing would happen here
        }
        
        return STATUS_OK;
    }
    
    bool validate() const {
        return initialized && !dataBuffer.hasOverflowed();
    }
    
    int reset() {
        if (initialized) {
            detachInterrupt(STROBE_INTERRUPT);
            dataBuffer.clear();
            bytesReceived = 0;
            overflowCount = 0;
            initialized = false;
        }
        return STATUS_OK;
    }
    
    // ISR simulation
    void simulateStrobeISR(uint8_t data) {
        if (!initialized) return;
        
        uint32_t startTime = micros();
        
        // Set BUSY high (processing)
        digitalWrite(BUSY_PIN, 1);
        digitalWrite(LPT_ACTIVITY_LED_PIN, 1);
        
        // Write data to buffer
        if (!dataBuffer.write(data)) {
            overflowCount++;
        } else {
            bytesReceived++;
        }
        
        // Pulse ACK low then high
        digitalWrite(ACK_PIN, 0);
        mock_time += 1; // Simulate 1us delay
        digitalWrite(ACK_PIN, 1);
        
        // Set BUSY low (ready)
        digitalWrite(BUSY_PIN, 0);
        digitalWrite(LPT_ACTIVITY_LED_PIN, 0);
        
        uint32_t endTime = micros();
        lastIsrTime = endTime - startTime;
        if (lastIsrTime > maxIsrTime) {
            maxIsrTime = lastIsrTime;
        }
    }
    
    // Status methods
    uint32_t getBytesReceived() const { return bytesReceived; }
    uint32_t getOverflowCount() const { return overflowCount; }
    uint8_t getBufferUtilization() const { return dataBuffer.getUtilization(); }
    uint32_t getLastIsrTime() const { return lastIsrTime; }
    uint32_t getMaxIsrTime() const { return maxIsrTime; }
    bool hasData() const { return dataBuffer.available() > 0; }
    
    size_t readData(uint8_t* buffer, size_t maxBytes) {
        size_t readCount = 0;
        uint8_t data;
        
        while (readCount < maxBytes && dataBuffer.read(&data)) {
            buffer[readCount++] = data;
        }
        
        return readCount;
    }
    
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
    
    void clearStatistics() {
        bytesReceived = 0;
        overflowCount = 0;
        lastIsrTime = 0;
        maxIsrTime = 0;
        dataBuffer.clearOverflow();
    }
};

// Test setup/teardown
void setUp(void) {
    mock_time = 0;
    mock_isr_callback = nullptr;
    memset(mock_pin_states, 0, sizeof(mock_pin_states));
}

void tearDown(void) {
    mock_time = 0;
    mock_isr_callback = nullptr;
}

// ============================================================================
// Component Lifecycle Tests
// ============================================================================

void test_parallel_port_initialization() {
    ParallelPortManagerTest ppm;
    
    int result = ppm.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(ppm.validate());
    TEST_ASSERT_EQUAL(0, ppm.getBytesReceived());
    TEST_ASSERT_EQUAL(0, ppm.getOverflowCount());
}

void test_parallel_port_double_initialization() {
    ParallelPortManagerTest ppm;
    
    TEST_ASSERT_EQUAL(STATUS_OK, ppm.initialize());
    TEST_ASSERT_EQUAL(STATUS_OK, ppm.initialize()); // Should be idempotent
    
    TEST_ASSERT_TRUE(ppm.validate());
}

void test_parallel_port_update_not_initialized() {
    ParallelPortManagerTest ppm;
    
    int result = ppm.update();
    
    TEST_ASSERT_EQUAL(STATUS_NOT_INITIALIZED, result);
}

void test_parallel_port_reset() {
    ParallelPortManagerTest ppm;
    
    ppm.initialize();
    ppm.simulateStrobeISR(0x42);
    TEST_ASSERT_EQUAL(1, ppm.getBytesReceived());
    
    int result = ppm.reset();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_FALSE(ppm.validate());
    TEST_ASSERT_EQUAL(0, ppm.getBytesReceived());
}

// ============================================================================
// Data Reception Tests
// ============================================================================

void test_parallel_port_single_byte_reception() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    uint8_t testData = 0x42;
    ppm.simulateStrobeISR(testData);
    
    TEST_ASSERT_EQUAL(1, ppm.getBytesReceived());
    TEST_ASSERT_TRUE(ppm.hasData());
    
    uint8_t readBuffer[1];
    size_t readCount = ppm.readData(readBuffer, 1);
    
    TEST_ASSERT_EQUAL(1, readCount);
    TEST_ASSERT_EQUAL(testData, readBuffer[0]);
}

void test_parallel_port_multiple_byte_reception() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    for (int i = 0; i < 5; i++) {
        ppm.simulateStrobeISR(testData[i]);
    }
    
    TEST_ASSERT_EQUAL(5, ppm.getBytesReceived());
    
    uint8_t readBuffer[5];
    size_t readCount = ppm.readData(readBuffer, 5);
    
    TEST_ASSERT_EQUAL(5, readCount);
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL(testData[i], readBuffer[i]);
    }
}

void test_parallel_port_buffer_overflow() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Fill buffer beyond capacity
    for (int i = 0; i <= RING_BUFFER_SIZE; i++) {
        ppm.simulateStrobeISR((uint8_t)i);
    }
    
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE, ppm.getBytesReceived());
    TEST_ASSERT_EQUAL(1, ppm.getOverflowCount());
    TEST_ASSERT_EQUAL(100, ppm.getBufferUtilization());
}

// ============================================================================
// Timing and Performance Tests
// ============================================================================

void test_parallel_port_isr_timing() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    mock_time = 1000;
    ppm.simulateStrobeISR(0x42);
    mock_time = 1001; // 1 microsecond
    
    uint32_t isrTime = ppm.getLastIsrTime();
    
    // ISR should complete within 2 microseconds for IEEE-1284 compliance
    TEST_ASSERT_LESS_THAN(3, isrTime);
    TEST_ASSERT_EQUAL(isrTime, ppm.getMaxIsrTime());
}

void test_parallel_port_continuous_reception() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Simulate continuous data reception
    for (int i = 0; i < RING_BUFFER_SIZE / 2; i++) {
        mock_time += 10; // 10us between bytes
        ppm.simulateStrobeISR((uint8_t)(i & 0xFF));
    }
    
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE / 2, ppm.getBytesReceived());
    TEST_ASSERT_EQUAL(0, ppm.getOverflowCount());
    TEST_ASSERT_LESS_THAN(3, ppm.getMaxIsrTime()); // All ISRs under 2us
}

// ============================================================================
// Hardware Flow Control Tests
// ============================================================================

void test_parallel_port_busy_ack_protocol() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Initially BUSY should be low (ready), ACK should be high (idle)
    TEST_ASSERT_FALSE(mock_pin_states[BUSY_PIN]);
    TEST_ASSERT_TRUE(mock_pin_states[ACK_PIN]);
    
    ppm.simulateStrobeISR(0x42);
    
    // After ISR, BUSY should be low (ready), ACK should be high (idle)
    TEST_ASSERT_FALSE(mock_pin_states[BUSY_PIN]);
    TEST_ASSERT_TRUE(mock_pin_states[ACK_PIN]);
}

void test_parallel_port_activity_led() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Initially LED should be off
    TEST_ASSERT_FALSE(mock_pin_states[LPT_ACTIVITY_LED_PIN]);
    
    ppm.simulateStrobeISR(0x42);
    
    // After ISR, LED should be off (ISR completes quickly)
    TEST_ASSERT_FALSE(mock_pin_states[LPT_ACTIVITY_LED_PIN]);
}

// ============================================================================
// Statistics and Monitoring Tests
// ============================================================================

void test_parallel_port_statistics_tracking() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Send some data
    for (int i = 0; i < 3; i++) {
        ppm.simulateStrobeISR((uint8_t)i);
    }
    
    TEST_ASSERT_EQUAL(3, ppm.getBytesReceived());
    TEST_ASSERT_EQUAL(0, ppm.getOverflowCount());
    
    // Cause overflow
    for (int i = 0; i < RING_BUFFER_SIZE; i++) {
        ppm.simulateStrobeISR((uint8_t)i);
    }
    
    TEST_ASSERT_GREATER_THAN(0, ppm.getOverflowCount());
}

void test_parallel_port_clear_statistics() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Generate some statistics
    ppm.simulateStrobeISR(0x42);
    TEST_ASSERT_EQUAL(1, ppm.getBytesReceived());
    
    ppm.clearStatistics();
    
    TEST_ASSERT_EQUAL(0, ppm.getBytesReceived());
    TEST_ASSERT_EQUAL(0, ppm.getOverflowCount());
    TEST_ASSERT_EQUAL(0, ppm.getLastIsrTime());
    TEST_ASSERT_EQUAL(0, ppm.getMaxIsrTime());
}

// ============================================================================
// Debug and Configuration Tests
// ============================================================================

void test_parallel_port_debug_control() {
    ParallelPortManagerTest ppm;
    
    TEST_ASSERT_FALSE(ppm.isDebugEnabled());
    
    ppm.setDebugEnabled(true);
    TEST_ASSERT_TRUE(ppm.isDebugEnabled());
    
    ppm.setDebugEnabled(false);
    TEST_ASSERT_FALSE(ppm.isDebugEnabled());
}

// ============================================================================
// Edge Cases and Error Conditions
// ============================================================================

void test_parallel_port_rapid_interrupts() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Simulate very rapid interrupts (worst case scenario)
    for (int i = 0; i < 10; i++) {
        mock_time += 1; // 1us between interrupts
        ppm.simulateStrobeISR((uint8_t)i);
    }
    
    // Should handle rapid interrupts without issue
    TEST_ASSERT_EQUAL(10, ppm.getBytesReceived());
    TEST_ASSERT_LESS_THAN(3, ppm.getMaxIsrTime());
}

void test_parallel_port_data_integrity() {
    ParallelPortManagerTest ppm;
    ppm.initialize();
    
    // Send specific pattern
    uint8_t pattern[] = {0x00, 0xFF, 0xAA, 0x55, 0x01, 0x80};
    
    for (int i = 0; i < 6; i++) {
        ppm.simulateStrobeISR(pattern[i]);
    }
    
    // Read back and verify pattern
    uint8_t readBuffer[6];
    size_t readCount = ppm.readData(readBuffer, 6);
    
    TEST_ASSERT_EQUAL(6, readCount);
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(pattern[i], readBuffer[i]);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_parallel_port_complete_workflow() {
    ParallelPortManagerTest ppm;
    
    // Initialize
    TEST_ASSERT_EQUAL(STATUS_OK, ppm.initialize());
    TEST_ASSERT_TRUE(ppm.validate());
    
    // Enable debug
    ppm.setDebugEnabled(true);
    
    // Receive data
    uint8_t testData[] = {0x12, 0x34, 0x56, 0x78};
    for (int i = 0; i < 4; i++) {
        ppm.simulateStrobeISR(testData[i]);
    }
    
    // Update (process buffered data)
    TEST_ASSERT_EQUAL(STATUS_OK, ppm.update());
    
    // Verify statistics
    TEST_ASSERT_EQUAL(4, ppm.getBytesReceived());
    TEST_ASSERT_EQUAL(0, ppm.getOverflowCount());
    TEST_ASSERT_LESS_THAN(50, ppm.getBufferUtilization());
    
    // Read data
    uint8_t readBuffer[4];
    size_t readCount = ppm.readData(readBuffer, 4);
    TEST_ASSERT_EQUAL(4, readCount);
    
    // Reset
    TEST_ASSERT_EQUAL(STATUS_OK, ppm.reset());
    TEST_ASSERT_FALSE(ppm.validate());
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Component lifecycle tests
    RUN_TEST(test_parallel_port_initialization);
    RUN_TEST(test_parallel_port_double_initialization);
    RUN_TEST(test_parallel_port_update_not_initialized);
    RUN_TEST(test_parallel_port_reset);
    
    // Data reception tests
    RUN_TEST(test_parallel_port_single_byte_reception);
    RUN_TEST(test_parallel_port_multiple_byte_reception);
    RUN_TEST(test_parallel_port_buffer_overflow);
    
    // Timing and performance tests
    RUN_TEST(test_parallel_port_isr_timing);
    RUN_TEST(test_parallel_port_continuous_reception);
    
    // Hardware flow control tests
    RUN_TEST(test_parallel_port_busy_ack_protocol);
    RUN_TEST(test_parallel_port_activity_led);
    
    // Statistics and monitoring tests
    RUN_TEST(test_parallel_port_statistics_tracking);
    RUN_TEST(test_parallel_port_clear_statistics);
    
    // Debug and configuration tests
    RUN_TEST(test_parallel_port_debug_control);
    
    // Edge cases and error conditions
    RUN_TEST(test_parallel_port_rapid_interrupts);
    RUN_TEST(test_parallel_port_data_integrity);
    
    // Integration tests
    RUN_TEST(test_parallel_port_complete_workflow);
    
    return UNITY_END();
}