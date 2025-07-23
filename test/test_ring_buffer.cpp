#include <unity.h>
#include <stdint.h>
#include <string.h>

// Mock definitions for testing
#define RING_BUFFER_SIZE 16

// RingBuffer class simulation for testing
class RingBufferTest {
private:
    uint8_t buffer[RING_BUFFER_SIZE];
    volatile size_t head;
    volatile size_t tail;
    volatile size_t count;
    volatile bool overflowFlag;
    
public:
    RingBufferTest() : head(0), tail(0), count(0), overflowFlag(false) {
        memset(buffer, 0, sizeof(buffer));
    }
    
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
        if (count == 0) {
            return false;
        }
        
        *data = buffer[tail];
        tail = (tail + 1) % RING_BUFFER_SIZE;
        count--;
        return true;
    }
    
    size_t available() const {
        return count;
    }
    
    size_t free() const {
        return RING_BUFFER_SIZE - count;
    }
    
    bool isEmpty() const {
        return count == 0;
    }
    
    bool isFull() const {
        return count >= RING_BUFFER_SIZE;
    }
    
    bool hasOverflowed() const {
        return overflowFlag;
    }
    
    void clearOverflow() {
        overflowFlag = false;
    }
    
    void clear() {
        head = 0;
        tail = 0;
        count = 0;
        overflowFlag = false;
        memset(buffer, 0, sizeof(buffer));
    }
    
    uint8_t getUtilization() const {
        return (count * 100) / RING_BUFFER_SIZE;
    }
    
    size_t writeArray(const uint8_t* data, size_t len) {
        size_t written = 0;
        for (size_t i = 0; i < len; i++) {
            if (write(data[i])) {
                written++;
            } else {
                break;
            }
        }
        return written;
    }
    
    size_t readArray(uint8_t* data, size_t maxLen) {
        size_t readCount = 0;
        for (size_t i = 0; i < maxLen; i++) {
            if (read(&data[i])) {
                readCount++;
            } else {
                break;
            }
        }
        return readCount;
    }
};

// Test setup/teardown
void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Basic Ring Buffer Tests
// ============================================================================

void test_ringbuffer_initialization() {
    RingBufferTest rb;
    
    TEST_ASSERT_TRUE(rb.isEmpty());
    TEST_ASSERT_FALSE(rb.isFull());
    TEST_ASSERT_EQUAL(0, rb.available());
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE, rb.free());
    TEST_ASSERT_FALSE(rb.hasOverflowed());
}

void test_ringbuffer_single_write_read() {
    RingBufferTest rb;
    uint8_t testData = 0x42;
    uint8_t readData;
    
    // Write single byte
    TEST_ASSERT_TRUE(rb.write(testData));
    TEST_ASSERT_FALSE(rb.isEmpty());
    TEST_ASSERT_EQUAL(1, rb.available());
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE - 1, rb.free());
    
    // Read single byte
    TEST_ASSERT_TRUE(rb.read(&readData));
    TEST_ASSERT_EQUAL(testData, readData);
    TEST_ASSERT_TRUE(rb.isEmpty());
    TEST_ASSERT_EQUAL(0, rb.available());
}

void test_ringbuffer_multiple_writes() {
    RingBufferTest rb;
    uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    // Write multiple bytes
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(rb.write(testData[i]));
    }
    
    TEST_ASSERT_EQUAL(5, rb.available());
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE - 5, rb.free());
    
    // Read back and verify
    uint8_t readData;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(rb.read(&readData));
        TEST_ASSERT_EQUAL(testData[i], readData);
    }
    
    TEST_ASSERT_TRUE(rb.isEmpty());
}

void test_ringbuffer_fill_to_capacity() {
    RingBufferTest rb;
    
    // Fill buffer to capacity
    for (int i = 0; i < RING_BUFFER_SIZE; i++) {
        TEST_ASSERT_TRUE(rb.write((uint8_t)(i & 0xFF)));
    }
    
    TEST_ASSERT_TRUE(rb.isFull());
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE, rb.available());
    TEST_ASSERT_EQUAL(0, rb.free());
    
    // Next write should fail and set overflow
    TEST_ASSERT_FALSE(rb.write(0xFF));
    TEST_ASSERT_TRUE(rb.hasOverflowed());
}

void test_ringbuffer_overflow_handling() {
    RingBufferTest rb;
    
    // Fill buffer beyond capacity
    for (int i = 0; i <= RING_BUFFER_SIZE; i++) {
        rb.write((uint8_t)i);
    }
    
    TEST_ASSERT_TRUE(rb.hasOverflowed());
    
    // Clear overflow flag
    rb.clearOverflow();
    TEST_ASSERT_FALSE(rb.hasOverflowed());
}

void test_ringbuffer_wraparound() {
    RingBufferTest rb;
    
    // Fill buffer
    for (int i = 0; i < RING_BUFFER_SIZE; i++) {
        rb.write((uint8_t)i);
    }
    
    // Read half
    uint8_t data;
    for (int i = 0; i < RING_BUFFER_SIZE / 2; i++) {
        TEST_ASSERT_TRUE(rb.read(&data));
        TEST_ASSERT_EQUAL(i, data);
    }
    
    // Write more data (should wrap around)
    for (int i = 0; i < RING_BUFFER_SIZE / 2; i++) {
        TEST_ASSERT_TRUE(rb.write((uint8_t)(0x80 + i)));
    }
    
    TEST_ASSERT_TRUE(rb.isFull());
    
    // Read remaining original data
    for (int i = RING_BUFFER_SIZE / 2; i < RING_BUFFER_SIZE; i++) {
        TEST_ASSERT_TRUE(rb.read(&data));
        TEST_ASSERT_EQUAL(i, data);
    }
    
    // Read wrapped data
    for (int i = 0; i < RING_BUFFER_SIZE / 2; i++) {
        TEST_ASSERT_TRUE(rb.read(&data));
        TEST_ASSERT_EQUAL((uint8_t)(0x80 + i), data);
    }
}

// ============================================================================
// Array Operations Tests
// ============================================================================

void test_ringbuffer_write_array() {
    RingBufferTest rb;
    uint8_t testData[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    
    size_t written = rb.writeArray(testData, sizeof(testData));
    
    TEST_ASSERT_EQUAL(5, written);
    TEST_ASSERT_EQUAL(5, rb.available());
    
    // Verify data
    uint8_t readData;
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(rb.read(&readData));
        TEST_ASSERT_EQUAL(testData[i], readData);
    }
}

void test_ringbuffer_read_array() {
    RingBufferTest rb;
    uint8_t testData[] = {0xA1, 0xB2, 0xC3, 0xD4};
    uint8_t readBuffer[10];
    
    // Write test data
    rb.writeArray(testData, sizeof(testData));
    
    // Read into array
    size_t readCount = rb.readArray(readBuffer, sizeof(readBuffer));
    
    TEST_ASSERT_EQUAL(4, readCount);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(testData[i], readBuffer[i]);
    }
    
    TEST_ASSERT_TRUE(rb.isEmpty());
}

void test_ringbuffer_partial_array_write() {
    RingBufferTest rb;
    uint8_t testData[RING_BUFFER_SIZE + 5];  // More than buffer capacity
    
    // Initialize test data
    for (int i = 0; i < sizeof(testData); i++) {
        testData[i] = (uint8_t)(i & 0xFF);
    }
    
    size_t written = rb.writeArray(testData, sizeof(testData));
    
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE, written);  // Should only write what fits
    TEST_ASSERT_TRUE(rb.isFull());
    TEST_ASSERT_TRUE(rb.hasOverflowed());
}

// ============================================================================
// Utilization and Status Tests  
// ============================================================================

void test_ringbuffer_utilization() {
    RingBufferTest rb;
    
    // Empty buffer
    TEST_ASSERT_EQUAL(0, rb.getUtilization());
    
    // 25% full
    for (int i = 0; i < RING_BUFFER_SIZE / 4; i++) {
        rb.write((uint8_t)i);
    }
    TEST_ASSERT_EQUAL(25, rb.getUtilization());
    
    // 50% full
    for (int i = 0; i < RING_BUFFER_SIZE / 4; i++) {
        rb.write((uint8_t)i);
    }
    TEST_ASSERT_EQUAL(50, rb.getUtilization());
    
    // 100% full
    for (int i = 0; i < RING_BUFFER_SIZE / 2; i++) {
        rb.write((uint8_t)i);
    }
    TEST_ASSERT_EQUAL(100, rb.getUtilization());
}

void test_ringbuffer_clear() {
    RingBufferTest rb;
    
    // Fill buffer and create overflow
    for (int i = 0; i <= RING_BUFFER_SIZE; i++) {
        rb.write((uint8_t)i);
    }
    
    TEST_ASSERT_TRUE(rb.isFull());
    TEST_ASSERT_TRUE(rb.hasOverflowed());
    
    // Clear buffer
    rb.clear();
    
    TEST_ASSERT_TRUE(rb.isEmpty());
    TEST_ASSERT_FALSE(rb.isFull());
    TEST_ASSERT_FALSE(rb.hasOverflowed());
    TEST_ASSERT_EQUAL(0, rb.available());
    TEST_ASSERT_EQUAL(RING_BUFFER_SIZE, rb.free());
    TEST_ASSERT_EQUAL(0, rb.getUtilization());
}

// ============================================================================
// Edge Cases and Error Conditions
// ============================================================================

void test_ringbuffer_read_empty() {
    RingBufferTest rb;
    uint8_t data;
    
    // Try to read from empty buffer
    TEST_ASSERT_FALSE(rb.read(&data));
    TEST_ASSERT_TRUE(rb.isEmpty());
}

void test_ringbuffer_write_full() {
    RingBufferTest rb;
    
    // Fill buffer completely
    for (int i = 0; i < RING_BUFFER_SIZE; i++) {
        TEST_ASSERT_TRUE(rb.write((uint8_t)i));
    }
    
    // Try to write to full buffer
    TEST_ASSERT_FALSE(rb.write(0xFF));
    TEST_ASSERT_TRUE(rb.hasOverflowed());
}

void test_ringbuffer_alternating_operations() {
    RingBufferTest rb;
    uint8_t data;
    
    // Alternating write/read operations
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_TRUE(rb.write((uint8_t)i));
        TEST_ASSERT_TRUE(rb.read(&data));
        TEST_ASSERT_EQUAL(i, data);
        TEST_ASSERT_TRUE(rb.isEmpty());
    }
}

void test_ringbuffer_stress_test() {
    RingBufferTest rb;
    const int iterations = 100;
    uint8_t writeData[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t readData[5];
    
    // Stress test with multiple write/read cycles
    for (int i = 0; i < iterations; i++) {
        // Write array
        size_t written = rb.writeArray(writeData, sizeof(writeData));
        TEST_ASSERT_EQUAL(5, written);
        
        // Read array back
        size_t readCount = rb.readArray(readData, sizeof(readData));
        TEST_ASSERT_EQUAL(5, readCount);
        
        // Verify data integrity
        for (int j = 0; j < 5; j++) {
            TEST_ASSERT_EQUAL(writeData[j], readData[j]);
        }
        
        TEST_ASSERT_TRUE(rb.isEmpty());
    }
}

// ============================================================================
// Performance-related Tests
// ============================================================================

void test_ringbuffer_concurrent_operations() {
    RingBufferTest rb;
    
    // Simulate concurrent producer/consumer
    for (int cycle = 0; cycle < 20; cycle++) {
        // Producer phase: fill 3/4 of buffer
        for (int i = 0; i < (RING_BUFFER_SIZE * 3) / 4; i++) {
            TEST_ASSERT_TRUE(rb.write((uint8_t)(cycle + i)));
        }
        
        // Consumer phase: read 1/2 of buffer
        uint8_t data;
        for (int i = 0; i < RING_BUFFER_SIZE / 2; i++) {
            TEST_ASSERT_TRUE(rb.read(&data));
        }
        
        // Verify buffer state is reasonable
        TEST_ASSERT_FALSE(rb.isEmpty());
        TEST_ASSERT_FALSE(rb.isFull());
    }
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_ringbuffer_initialization);
    RUN_TEST(test_ringbuffer_single_write_read);
    RUN_TEST(test_ringbuffer_multiple_writes);
    RUN_TEST(test_ringbuffer_fill_to_capacity);
    RUN_TEST(test_ringbuffer_overflow_handling);
    RUN_TEST(test_ringbuffer_wraparound);
    
    // Array operations tests
    RUN_TEST(test_ringbuffer_write_array);
    RUN_TEST(test_ringbuffer_read_array);
    RUN_TEST(test_ringbuffer_partial_array_write);
    
    // Status and utilization tests
    RUN_TEST(test_ringbuffer_utilization);
    RUN_TEST(test_ringbuffer_clear);
    
    // Edge cases and error conditions
    RUN_TEST(test_ringbuffer_read_empty);
    RUN_TEST(test_ringbuffer_write_full);
    RUN_TEST(test_ringbuffer_alternating_operations);
    RUN_TEST(test_ringbuffer_stress_test);
    
    // Performance tests
    RUN_TEST(test_ringbuffer_concurrent_operations);
    
    return UNITY_END();
}