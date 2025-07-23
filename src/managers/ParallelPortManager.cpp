#include "ParallelPortManager.h"
#include "MemoryUtils.h"
#include "ServiceLocator.h"

// Global pointer for ISR access
static ParallelPortManager* g_parallelPortManager = nullptr;

ParallelPortManager::ParallelPortManager() 
    : initialized(false), captureEnabled(false), bytesReceived(0), 
      overflowCount(0), lastInterruptTime(0), debugEnabled(false),
      busyAsserted(false), errorState(false), totalInterrupts(0),
      maxISRTime(0), avgISRTime(0) {
    g_parallelPortManager = this;
}

int ParallelPortManager::initialize() {
    if (initialized) {
        return STATUS_OK;
    }
    
    // Configure parallel port pins
    configurePins();
    
    // Initialize ring buffer
    ringBuffer.clear();
    
    // Set initial signal states
    digitalWrite(LPT_BUSY_PIN, LOW);        // Not busy initially
    digitalWrite(LPT_ACKNOWLEDGE_PIN, HIGH); // Acknowledge inactive (HIGH)
    digitalWrite(LPT_PAPER_OUT_PIN, LOW);   // Paper out forced low
    digitalWrite(LPT_SELECT_PIN, HIGH);     // Select forced high
    digitalWrite(LPT_ERROR_PIN, HIGH);      // No error initially
    
    // Reset statistics
    bytesReceived = 0;
    overflowCount = 0;
    totalInterrupts = 0;
    maxISRTime = 0;
    avgISRTime = 0;
    
    // Attach interrupt handler
    attachInterrupt(LPT_STROBE_INTERRUPT, parallelPortISR, FALLING);
    
    initialized = true;
    captureEnabled = true;
    
    if (debugEnabled) {
        Serial.print(F("ParallelPortManager: Initialized on pin "));
        Serial.println(LPT_STROBE_PIN);
    }
    
    return STATUS_OK;
}

int ParallelPortManager::update() {
    if (!initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    // Check for buffer overflow
    if (ringBuffer.hasOverflow()) {
        overflowCount++;
        if (debugEnabled) {
            Serial.println(F("ParallelPortManager: Buffer overflow detected"));
        }
        ringBuffer.clearOverflow();
    }
    
    // Update LED indicators
    digitalWrite(LPT_ACTIVITY_LED_PIN, ringBuffer.available() > 0 ? HIGH : LOW);
    
    return STATUS_OK;
}

int ParallelPortManager::getStatus() const {
    if (!initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    if (errorState) {
        return STATUS_ERROR;
    }
    
    if (ringBuffer.hasOverflow()) {
        return STATUS_ERROR;
    }
    
    return STATUS_OK;
}

const __FlashStringHelper* ParallelPortManager::getName() const {
    return F("ParallelPortManager");
}

bool ParallelPortManager::validate() const {
    return initialized && 
           validateMemory(&ringBuffer, sizeof(ringBuffer)) &&
           !errorState;
}

int ParallelPortManager::reset() {
    if (initialized) {
        // Detach interrupt
        detachInterrupt(LPT_STROBE_INTERRUPT);
        
        // Clear buffer
        ringBuffer.clear();
        
        // Reset statistics
        bytesReceived = 0;
        overflowCount = 0;
        totalInterrupts = 0;
        maxISRTime = 0;
        avgISRTime = 0;
        
        // Reset state
        captureEnabled = false;
        errorState = false;
        busyAsserted = false;
        
        initialized = false;
    }
    
    return initialize();
}

size_t ParallelPortManager::getMemoryUsage() const {
    return sizeof(*this) + sizeof(ringBuffer);
}

void ParallelPortManager::setDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

bool ParallelPortManager::isDebugEnabled() const {
    return debugEnabled;
}

void ParallelPortManager::configurePins() {
    // Configure data pins as inputs with pull-ups
    pinMode(LPT_DATA0_PIN, INPUT_PULLUP);
    pinMode(LPT_DATA1_PIN, INPUT_PULLUP);
    pinMode(LPT_DATA2_PIN, INPUT_PULLUP);
    pinMode(LPT_DATA3_PIN, INPUT_PULLUP);
    pinMode(LPT_DATA4_PIN, INPUT_PULLUP);
    pinMode(LPT_DATA5_PIN, INPUT_PULLUP);
    pinMode(LPT_DATA6_PIN, INPUT_PULLUP);
    pinMode(LPT_DATA7_PIN, INPUT_PULLUP);
    
    // Configure control pins as inputs
    pinMode(LPT_STROBE_PIN, INPUT_PULLUP);
    pinMode(LPT_AUTO_FEED_PIN, INPUT_PULLUP);
    pinMode(LPT_INITIALIZE_PIN, INPUT_PULLUP);
    pinMode(LPT_SELECT_IN_PIN, INPUT_PULLUP);
    
    // Configure status pins as outputs
    pinMode(LPT_ACKNOWLEDGE_PIN, OUTPUT);
    pinMode(LPT_BUSY_PIN, OUTPUT);
    pinMode(LPT_PAPER_OUT_PIN, OUTPUT);
    pinMode(LPT_SELECT_PIN, OUTPUT);
    pinMode(LPT_ERROR_PIN, OUTPUT);
    
    // Configure LED pins
    pinMode(LPT_ACTIVITY_LED_PIN, OUTPUT);
    digitalWrite(LPT_ACTIVITY_LED_PIN, LOW);
}

void ParallelPortManager::assertFlowControl() {
    digitalWrite(LPT_BUSY_PIN, HIGH);
    busyAsserted = true;
}

void ParallelPortManager::releaseFlowControl() {
    digitalWrite(LPT_BUSY_PIN, LOW);
    busyAsserted = false;
}

void ParallelPortManager::updateTimingStats(uint16_t executionTime) {
    if (executionTime > maxISRTime) {
        maxISRTime = executionTime;
    }
    
    // Update running average (simple moving average)
    avgISRTime = (avgISRTime + executionTime) / 2;
}

void ParallelPortManager::setCaptureEnabled(bool enabled) {
    captureEnabled = enabled;
    
    if (debugEnabled) {
        Serial.print(F("ParallelPortManager: Capture "));
        Serial.println(enabled ? F("enabled") : F("disabled"));
    }
}

bool ParallelPortManager::isCaptureEnabled() const {
    return captureEnabled;
}

size_t ParallelPortManager::getAvailableBytes() const {
    return ringBuffer.available();
}

size_t ParallelPortManager::readData(uint8_t* dest, size_t maxBytes) {
    if (!dest || !initialized) {
        return 0;
    }
    
    return ringBuffer.readBytes(dest, maxBytes);
}

bool ParallelPortManager::peekData(uint8_t& data) const {
    return ringBuffer.peek(data);
}

void ParallelPortManager::clearBuffer() {
    ringBuffer.clear();
    
    if (debugEnabled) {
        Serial.println(F("ParallelPortManager: Buffer cleared"));
    }
}

uint8_t ParallelPortManager::getBufferUtilization() const {
    return ringBuffer.getUtilization();
}

bool ParallelPortManager::hasBufferOverflow() const {
    return ringBuffer.hasOverflow();
}

void ParallelPortManager::clearBufferOverflow() {
    ringBuffer.clearOverflow();
}

uint32_t ParallelPortManager::getTotalBytesReceived() const {
    return bytesReceived;
}

uint32_t ParallelPortManager::getOverflowCount() const {
    return overflowCount;
}

void ParallelPortManager::getInterruptStats(uint32_t& totalInts, uint16_t& maxTime, uint16_t& avgTime) const {
    totalInts = totalInterrupts;
    maxTime = maxISRTime;
    avgTime = avgISRTime;
}

uint32_t ParallelPortManager::testInterrupt(uint32_t durationMs) {
    uint32_t startCount = totalInterrupts;
    uint32_t startTime = millis();
    
    Serial.print(F("Testing interrupts for "));
    Serial.print(durationMs);
    Serial.println(F(" ms..."));
    
    while (millis() - startTime < durationMs) {
        delay(100);
        Serial.print(F("."));
    }
    
    uint32_t endCount = totalInterrupts;
    Serial.println();
    Serial.print(F("Received "));
    Serial.print(endCount - startCount);
    Serial.println(F(" interrupts"));
    
    return endCount - startCount;
}

bool ParallelPortManager::testProtocolSignals() {
    if (!initialized) {
        return false;
    }
    
    Serial.println(F("Testing LPT protocol signals..."));
    
    // Test Busy signal
    digitalWrite(LPT_BUSY_PIN, HIGH);
    delayMicroseconds(HARDWARE_DELAY);
    bool busyHigh = digitalRead(LPT_BUSY_PIN);
    
    digitalWrite(LPT_BUSY_PIN, LOW);
    delayMicroseconds(HARDWARE_DELAY);
    bool busyLow = !digitalRead(LPT_BUSY_PIN);
    
    // Test Acknowledge signal
    digitalWrite(LPT_ACKNOWLEDGE_PIN, LOW);
    delayMicroseconds(ACK_PULSE_WIDTH);
    digitalWrite(LPT_ACKNOWLEDGE_PIN, HIGH);
    
    // Test Error signal
    digitalWrite(LPT_ERROR_PIN, LOW);
    delayMicroseconds(HARDWARE_DELAY);
    bool errorLow = !digitalRead(LPT_ERROR_PIN);
    
    digitalWrite(LPT_ERROR_PIN, HIGH);
    delayMicroseconds(HARDWARE_DELAY);
    bool errorHigh = digitalRead(LPT_ERROR_PIN);
    
    bool allTestsPassed = busyHigh && busyLow && errorLow && errorHigh;
    
    Serial.print(F("Signal tests: "));
    Serial.println(allTestsPassed ? F("PASSED") : F("FAILED"));
    
    return allTestsPassed;
}

void ParallelPortManager::getPortStatus(bool& busy, bool& ack, bool& error) const {
    busy = busyAsserted;
    ack = !digitalRead(LPT_ACKNOWLEDGE_PIN); // Active LOW
    error = !digitalRead(LPT_ERROR_PIN);     // Active LOW
}

void ParallelPortManager::handleInterrupt() {
    // CRITICAL: This function must execute in ≤2μs for IEEE-1284 compliance
    uint32_t startTime = micros();
    
    if (!captureEnabled || !initialized) {
        return;
    }
    
    // Assert BUSY signal immediately
    digitalWrite(LPT_BUSY_PIN, HIGH);
    
    // Hardware delay for data stability (5μs per spec)
    delayMicroseconds(HARDWARE_DELAY);
    
    // Read 8-bit parallel data atomically
    uint8_t data = 0;
    data |= (digitalRead(LPT_DATA0_PIN) ? 0x01 : 0x00);
    data |= (digitalRead(LPT_DATA1_PIN) ? 0x02 : 0x00);
    data |= (digitalRead(LPT_DATA2_PIN) ? 0x04 : 0x00);
    data |= (digitalRead(LPT_DATA3_PIN) ? 0x08 : 0x00);
    data |= (digitalRead(LPT_DATA4_PIN) ? 0x10 : 0x00);
    data |= (digitalRead(LPT_DATA5_PIN) ? 0x20 : 0x00);
    data |= (digitalRead(LPT_DATA6_PIN) ? 0x40 : 0x00);
    data |= (digitalRead(LPT_DATA7_PIN) ? 0x80 : 0x00);
    
    // Write to ring buffer
    if (ringBuffer.write(data)) {
        bytesReceived++;
    }
    
    // Send acknowledge pulse (20μs per TDS2024 requirements)
    digitalWrite(LPT_ACKNOWLEDGE_PIN, LOW);
    delayMicroseconds(ACK_PULSE_WIDTH);
    digitalWrite(LPT_ACKNOWLEDGE_PIN, HIGH);
    
    // Release BUSY signal
    digitalWrite(LPT_BUSY_PIN, LOW);
    
    // Update statistics
    totalInterrupts++;
    uint32_t endTime = micros();
    uint16_t executionTime = (uint16_t)(endTime - startTime);
    updateTimingStats(executionTime);
    
    lastInterruptTime = endTime;
}

void ParallelPortManager::setErrorState(bool error) {
    errorState = error;
    digitalWrite(LPT_ERROR_PIN, error ? LOW : HIGH); // Active LOW
}

// Global ISR function
void parallelPortISR() {
    if (g_parallelPortManager != nullptr) {
        g_parallelPortManager->handleInterrupt();
    }
}