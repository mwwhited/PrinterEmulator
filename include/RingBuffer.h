#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <Arduino.h>
#include "HardwareConfig.h"

/**
 * High-performance ring buffer for parallel port data capture
 * Designed for interrupt service routine usage with ≤2μs constraints
 * Thread-safe for single producer, single consumer scenario
 */
class RingBuffer {
private:
    uint8_t buffer[RING_BUFFER_SIZE];
    volatile size_t head;           // Write position (producer)
    volatile size_t tail;           // Read position (consumer)
    volatile size_t count;          // Current number of elements
    volatile bool overflowFlag;     // Overflow detection
    
public:
    /**
     * Constructor - initializes empty buffer
     */
    RingBuffer();
    
    /**
     * Write single byte to buffer (ISR-safe)
     * @param data Byte to write
     * @return true if write successful, false if buffer full
     */
    bool write(uint8_t data);
    
    /**
     * Read single byte from buffer
     * @param data Reference to store read byte
     * @return true if read successful, false if buffer empty
     */
    bool read(uint8_t& data);
    
    /**
     * Peek at next byte without removing it
     * @param data Reference to store peeked byte
     * @return true if peek successful, false if buffer empty
     */
    bool peek(uint8_t& data) const;
    
    /**
     * Get number of bytes available for reading
     * @return Number of bytes in buffer
     */
    size_t available() const;
    
    /**
     * Get number of free bytes in buffer
     * @return Number of free bytes
     */
    size_t free() const;
    
    /**
     * Check if buffer is empty
     * @return true if buffer is empty
     */
    bool isEmpty() const;
    
    /**
     * Check if buffer is full
     * @return true if buffer is full
     */
    bool isFull() const;
    
    /**
     * Clear buffer (reset to empty state)
     */
    void clear();
    
    /**
     * Check if overflow occurred
     * @return true if overflow detected
     */
    bool hasOverflow() const;
    
    /**
     * Clear overflow flag
     */
    void clearOverflow();
    
    /**
     * Get buffer capacity
     * @return Total buffer size
     */
    size_t capacity() const;
    
    /**
     * Read multiple bytes from buffer
     * @param dest Destination buffer
     * @param maxBytes Maximum bytes to read
     * @return Number of bytes actually read
     */
    size_t readBytes(uint8_t* dest, size_t maxBytes);
    
    /**
     * Write multiple bytes to buffer
     * @param src Source buffer
     * @param numBytes Number of bytes to write
     * @return Number of bytes actually written
     */
    size_t writeBytes(const uint8_t* src, size_t numBytes);
    
    /**
     * Get buffer utilization percentage
     * @return Utilization (0-100)
     */
    uint8_t getUtilization() const;
};

#endif // RINGBUFFER_H