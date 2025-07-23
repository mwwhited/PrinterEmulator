#ifndef PARALLELPORTMANAGER_H
#define PARALLELPORTMANAGER_H

#include <Arduino.h>
#include "IComponent.h"
#include "RingBuffer.h"
#include "HardwareConfig.h"

/**
 * ParallelPortManager - IEEE-1284 Standard Parallel Port Manager
 * Handles real-time data capture from Tektronix TDS2024 oscilloscope
 * Implements hardware flow control with ≤2μs ISR constraints
 */
class ParallelPortManager : public IComponent {
private:
    RingBuffer ringBuffer;              // 512-byte ring buffer for data
    volatile bool initialized;          // Initialization state
    volatile bool captureEnabled;       // Data capture enabled flag
    volatile uint32_t bytesReceived;    // Total bytes received counter
    volatile uint32_t overflowCount;    // Overflow events counter
    volatile uint32_t lastInterruptTime; // Last interrupt timestamp (μs)
    bool debugEnabled;                  // Debug output enabled
    
    // Hardware control state
    volatile bool busyAsserted;         // Busy signal state
    volatile bool errorState;           // Error condition detected
    
    // Statistics
    volatile uint32_t totalInterrupts;  // Total interrupt count
    volatile uint16_t maxISRTime;       // Maximum ISR execution time (μs)
    volatile uint16_t avgISRTime;       // Average ISR execution time (μs)
    
    /**
     * Configure parallel port pins
     */
    void configurePins();
    
    /**
     * Assert hardware flow control signals
     */
    void assertFlowControl();
    
    /**
     * Release hardware flow control signals
     */
    void releaseFlowControl();
    
    /**
     * Update timing statistics
     * @param executionTime ISR execution time in microseconds
     */
    void updateTimingStats(uint16_t executionTime);
    
public:
    /**
     * Constructor
     */
    ParallelPortManager();
    
    /**
     * Destructor
     */
    ~ParallelPortManager() override = default;
    
    // IComponent interface implementation
    int initialize() override;
    int update() override;
    int getStatus() const override;
    const __FlashStringHelper* getName() const override;
    bool validate() const override;
    int reset() override;
    size_t getMemoryUsage() const override;
    void setDebugEnabled(bool enabled) override;
    bool isDebugEnabled() const override;
    
    /**
     * Enable/disable data capture
     * @param enabled true to enable capture
     */
    void setCaptureEnabled(bool enabled);
    
    /**
     * Check if data capture is enabled
     * @return true if capture enabled
     */
    bool isCaptureEnabled() const;
    
    /**
     * Get available data bytes
     * @return Number of bytes available for reading
     */
    size_t getAvailableBytes() const;
    
    /**
     * Read data from buffer
     * @param dest Destination buffer
     * @param maxBytes Maximum bytes to read
     * @return Number of bytes actually read
     */
    size_t readData(uint8_t* dest, size_t maxBytes);
    
    /**
     * Peek at next byte without removing it
     * @param data Reference to store peeked byte
     * @return true if peek successful
     */
    bool peekData(uint8_t& data) const;
    
    /**
     * Clear data buffer
     */
    void clearBuffer();
    
    /**
     * Get buffer utilization percentage
     * @return Utilization (0-100)
     */
    uint8_t getBufferUtilization() const;
    
    /**
     * Check if buffer overflow occurred
     * @return true if overflow detected
     */
    bool hasBufferOverflow() const;
    
    /**
     * Clear buffer overflow flag
     */
    void clearBufferOverflow();
    
    /**
     * Get total bytes received
     * @return Total bytes received since initialization
     */
    uint32_t getTotalBytesReceived() const;
    
    /**
     * Get overflow count
     * @return Number of overflow events
     */
    uint32_t getOverflowCount() const;
    
    /**
     * Get interrupt statistics
     * @param totalInts Total interrupt count
     * @param maxTime Maximum ISR time (μs)
     * @param avgTime Average ISR time (μs)
     */
    void getInterruptStats(uint32_t& totalInts, uint16_t& maxTime, uint16_t& avgTime) const;
    
    /**
     * Test interrupt for specified duration
     * @param durationMs Test duration in milliseconds
     * @return Number of interrupts received during test
     */
    uint32_t testInterrupt(uint32_t durationMs);
    
    /**
     * Test LPT protocol signals
     * @return true if all signals respond correctly
     */
    bool testProtocolSignals();
    
    /**
     * Get current parallel port status
     * @param busy Busy signal state
     * @param ack Acknowledge signal state
     * @param error Error signal state
     */
    void getPortStatus(bool& busy, bool& ack, bool& error) const;
    
    /**
     * Interrupt Service Routine (called by hardware interrupt)
     * CRITICAL: Must execute in ≤2μs for IEEE-1284 compliance
     */
    void handleInterrupt();
    
    /**
     * Force error state (for testing)
     * @param error true to set error state
     */
    void setErrorState(bool error);
};

// Global interrupt handler function
void parallelPortISR();

#endif // PARALLELPORTMANAGER_H