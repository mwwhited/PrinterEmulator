#include "RingBuffer.h"

RingBuffer::RingBuffer() 
    : head(0), tail(0), count(0), overflowFlag(false) {
    // Buffer is automatically zero-initialized
}

bool RingBuffer::write(uint8_t data) {
    // Check if buffer is full
    if (count >= RING_BUFFER_SIZE) {
        overflowFlag = true;
        return false;
    }
    
    // Write data at head position
    buffer[head] = data;
    
    // Update head with wrap-around
    head = (head + 1) % RING_BUFFER_SIZE;
    
    // Increment count atomically
    count++;
    
    return true;
}

bool RingBuffer::read(uint8_t& data) {
    // Check if buffer is empty
    if (count == 0) {
        return false;
    }
    
    // Read data from tail position
    data = buffer[tail];
    
    // Update tail with wrap-around
    tail = (tail + 1) % RING_BUFFER_SIZE;
    
    // Decrement count atomically
    count--;
    
    return true;
}

bool RingBuffer::peek(uint8_t& data) const {
    // Check if buffer is empty
    if (count == 0) {
        return false;
    }
    
    // Peek at data from tail position without moving tail
    data = buffer[tail];
    return true;
}

size_t RingBuffer::available() const {
    return count;
}

size_t RingBuffer::free() const {
    return RING_BUFFER_SIZE - count;
}

bool RingBuffer::isEmpty() const {
    return count == 0;
}

bool RingBuffer::isFull() const {
    return count >= RING_BUFFER_SIZE;
}

void RingBuffer::clear() {
    // Disable interrupts during clear to ensure atomicity
    noInterrupts();
    head = 0;
    tail = 0;
    count = 0;
    overflowFlag = false;
    interrupts();
}

bool RingBuffer::hasOverflow() const {
    return overflowFlag;
}

void RingBuffer::clearOverflow() {
    overflowFlag = false;
}

size_t RingBuffer::capacity() const {
    return RING_BUFFER_SIZE;
}

size_t RingBuffer::readBytes(uint8_t* dest, size_t maxBytes) {
    if (!dest || maxBytes == 0) {
        return 0;
    }
    
    size_t bytesRead = 0;
    
    while (bytesRead < maxBytes && count > 0) {
        dest[bytesRead] = buffer[tail];
        tail = (tail + 1) % RING_BUFFER_SIZE;
        count--;
        bytesRead++;
    }
    
    return bytesRead;
}

size_t RingBuffer::writeBytes(const uint8_t* src, size_t numBytes) {
    if (!src || numBytes == 0) {
        return 0;
    }
    
    size_t bytesWritten = 0;
    
    while (bytesWritten < numBytes && count < RING_BUFFER_SIZE) {
        buffer[head] = src[bytesWritten];
        head = (head + 1) % RING_BUFFER_SIZE;
        count++;
        bytesWritten++;
    }
    
    // Set overflow flag if we couldn't write all bytes
    if (bytesWritten < numBytes) {
        overflowFlag = true;
    }
    
    return bytesWritten;
}

uint8_t RingBuffer::getUtilization() const {
    return (uint8_t)((count * 100) / RING_BUFFER_SIZE);
}