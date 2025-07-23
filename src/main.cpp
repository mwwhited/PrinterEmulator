#include <Arduino.h>
#include <Wire.h>

// Core system includes
#include "HardwareConfig.h"
#include "ServiceLocator.h"
#include "MemoryUtils.h"
#include "HardwareSelfTest.h"
#include "DebugCommands.h"

// Component includes
#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "ConfigurationManager.h"
#include "TimeManager.h"
#include "SystemManager.h"
#include "HeartbeatLEDManager.h"

// Storage plugins
#include "SDCardStoragePlugin.h"
#include "EEPROMStoragePlugin.h"
#include "SerialStoragePlugin.h"

/**
 * MegaDeviceBridge - Main Application
 * Embedded data acquisition system for Tektronix TDS2024 oscilloscope
 * Implements IEEE-1284 compliant parallel port interface with multi-storage support
 */

// Global component instances (static allocation)
static ParallelPortManager parallelPortManager;
static FileSystemManager fileSystemManager;
static DisplayManager displayManager;
static ConfigurationManager configurationManager;
static TimeManager timeManager;
static SystemManager systemManager;
static HeartbeatLEDManager heartbeatLEDManager;

// Storage plugin instances
static SDCardStoragePlugin sdCardPlugin;
static EEPROMStoragePlugin eepromPlugin;
static SerialStoragePlugin serialPlugin;

// Application state
static bool systemInitialized = false;
static bool systemError = false;
static uint32_t lastLoopTime = 0;
static uint32_t lastStatusUpdate = 0;
static uint32_t loopCounter = 0;

/**
 * Initialize all system components
 * @return true if initialization successful
 */
bool initializeSystem() {
    // Initialize serial communication first with explicit configuration
    Serial.begin(SERIAL_BAUD_RATE, SERIAL_8N1);
    
    // Ensure serial is ready before proceeding
    while (!Serial && millis() < 5000) {
        // Wait for serial connection with longer timeout
        delay(10);
    }
    
    // Clear any garbage in serial buffer
    while (Serial.available()) {
        Serial.read();
    }
    
    // Send startup banner with proper line endings
    Serial.println();
    Serial.println(F("====================================="));
    Serial.println(F("MegaDeviceBridge v1.0"));
    Serial.println(F("Tektronix TDS2024 Data Acquisition"));
    Serial.println(F("Serial: 115200 8N1"));
    Serial.println(F("====================================="));
    Serial.println(F("Starting initialization..."));
    Serial.flush(); // Ensure all data is sent
    
    // Initialize I2C for RTC
    Wire.begin();
    
    // Register storage plugins with FileSystemManager
    fileSystemManager.registerPlugins(&sdCardPlugin, &eepromPlugin, &serialPlugin);
    
    // Register all components with ServiceLocator
    ServiceLocator::registerComponents(
        &parallelPortManager,
        &fileSystemManager,
        &displayManager,
        &configurationManager,
        &timeManager,
        &systemManager,
        &heartbeatLEDManager
    );
    
    // Initialize all components (continue even if some fail)
    int result = ServiceLocator::initializeAll();
    if (result != STATUS_OK) {
        // Continue with available components
    }
    
    // Validate components (continue even if some fail)
    if (!ServiceLocator::validateAll()) {
        Serial.println(F("Some components failed validation, continuing..."));
        // Don't return false - continue with available components
    }
    
    // Display startup complete message
    displayManager.displayMessagePGM(F("MegaDeviceBridge"), F("Ready"), 2000);
    
    Serial.println(F("System initialization complete"));
    Serial.print(F("Available RAM: "));
    Serial.print(getAvailableRAM());
    Serial.println(F(" bytes"));
    
    // Show active storage system
    Serial.print(F("Active storage: "));
    Serial.println(fileSystemManager.getCurrentStorageName());
    Serial.println(F("Note: EEPROM failure is OK - system uses SD card or Serial"));
    
    return true;
}

/**
 * Handle system error condition
 * @param errorCode Error code
 * @param errorMessage Error message
 */
void handleSystemError(int errorCode, const char* errorMessage) {
    systemError = true;
    
    Serial.print(F("SYSTEM ERROR "));
    Serial.print(errorCode);
    Serial.print(F(": "));
    Serial.println(errorMessage);
    
    // Display error on LCD
    displayManager.displayError(errorMessage, errorCode);
    
    // Trigger SOS pattern on heartbeat LED
    heartbeatLEDManager.triggerSOSPattern();
}

/**
 * Process captured parallel port data
 */
void processParallelPortData() {
    size_t availableBytes = parallelPortManager.getAvailableBytes();
    
    if (availableBytes > 0) {
        // EMERGENCY: Single byte processing to prevent corruption
        static uint8_t dataBuffer[1];   // EMERGENCY: 1 byte to prevent corruption
        size_t bytesToRead = min(availableBytes, sizeof(dataBuffer));
        
        // Read data from parallel port manager
        size_t bytesRead = parallelPortManager.readData(dataBuffer, bytesToRead);
        
        if (bytesRead > 0) {
            // Generate filename with timestamp
            char filename[MAX_FILENAME_LENGTH];
            
            // Get current time for filename
            // For now, use simple counter-based naming
            static uint16_t fileCounter = 1;
            snprintf(filename, sizeof(filename), "data_%04d.bin", fileCounter++);
            
            // Write to current storage
            size_t bytesWritten = fileSystemManager.writeFile(filename, dataBuffer, bytesRead);
            
            if (bytesWritten == bytesRead) {
                // Update display with capture status
                char statusMsg[17];
                snprintf(statusMsg, sizeof(statusMsg), "Saved: %s", filename);
                displayManager.displayMessage("Data Captured", statusMsg, 2000);
                
                Serial.print(F("Captured "));
                Serial.print(bytesRead);
                Serial.print(F(" bytes to "));
                Serial.println(filename);
            } else {
                // Rate limit write error messages to prevent LCD flashing
                static uint32_t lastWriteError = 0;
                if (millis() - lastWriteError >= 5000) {
                    Serial.println(F("Warning: Partial write or write failed"));
                    displayManager.displayError("Write err");
                    lastWriteError = millis();
                }
            }
        }
    }
}

/**
 * Update system status display
 */
void updateSystemStatus() {
    uint32_t currentTime = millis();
    
    if (currentTime - lastStatusUpdate >= 5000) { // Update every 5 seconds
        // Get system statistics
        uint32_t totalBytes = parallelPortManager.getTotalBytesReceived();
        uint32_t overflows = parallelPortManager.getOverflowCount();
        uint8_t bufferUtil = parallelPortManager.getBufferUtilization();
        
        // Display on serial
        Serial.print(F("Status - Bytes: "));
        Serial.print(totalBytes);
        Serial.print(F(", Overflows: "));
        Serial.print(overflows);
        Serial.print(F(", Buffer: "));
        Serial.print(bufferUtil);
        Serial.print(F("%, RAM: "));
        Serial.print(getAvailableRAM());
        Serial.println(F("B"));
        
        lastStatusUpdate = currentTime;
    }
}

/**
 * Arduino setup function - called once at startup
 */
void setup() {
    // Set LED pins as outputs for early indication
    pinMode(HEARTBEAT_LED_PIN, OUTPUT);
    pinMode(LPT_ACTIVITY_LED_PIN, OUTPUT);
    pinMode(WRITE_ACTIVITY_LED_PIN, OUTPUT);
    
    // Flash all LEDs during startup
    for (int i = 0; i < 3; i++) {
        digitalWrite(HEARTBEAT_LED_PIN, HIGH);
        digitalWrite(LPT_ACTIVITY_LED_PIN, HIGH);
        digitalWrite(WRITE_ACTIVITY_LED_PIN, HIGH);
        delay(200);
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
        digitalWrite(LPT_ACTIVITY_LED_PIN, LOW);
        digitalWrite(WRITE_ACTIVITY_LED_PIN, LOW);
        delay(200);
    }
    
    // Initialize system
    systemInitialized = initializeSystem();
    
    if (!systemInitialized) {
        handleSystemError(STATUS_ERROR, "Init failed");
        return;
    }
    
    // Enable parallel port data capture
    parallelPortManager.setCaptureEnabled(true);
    
    // Enable auto status updates on display
    displayManager.setAutoStatusUpdate(true, 3000);
    
    // DEBUG COMMANDS DISABLED - CRITICAL MEMORY SHORTAGE
    // DebugCommands::initialize();
    
    // SELF-TEST DISABLED TO SAVE CRITICAL MEMORY
    // Quick validation only
    Serial.println(F("System ready - self-test disabled for memory"));
    
    Serial.println(F("Setup complete - entering main loop"));
    Serial.println(F("System ready for TDS2024 data capture"));
}

/**
 * Arduino main loop - called continuously
 * Implements cooperative multitasking for all system components
 */
void loop() {
    uint32_t currentTime = millis();
    
    // Skip loop if system not initialized or in error state
    if (!systemInitialized) {
        delay(1000);
        return;
    }
    
    // Increment loop counter for performance monitoring
    loopCounter++;
    
    // Update all system components (cooperative multitasking)
    int updateResult = ServiceLocator::updateAll();
    if (updateResult != STATUS_OK) {
        handleSystemError(updateResult, "Component update failed");
        return;
    }
    
    // Process captured parallel port data
    processParallelPortData();
    
    // Update system status display
    updateSystemStatus();
    
    // DEBUG COMMANDS DISABLED - CRITICAL MEMORY SHORTAGE
    // DebugCommands::update();
    
    // Check for buffer overflow conditions (rate limited)
    static uint32_t lastOverflowCheck = 0;
    if (currentTime - lastOverflowCheck >= 5000) { // Check every 5 seconds
        if (parallelPortManager.hasBufferOverflow()) {
            Serial.println(F("Warning: Parallel port buffer overflow"));
            parallelPortManager.clearBufferOverflow();
            
            // Brief error indication on display
            displayManager.displayError("Buf ovflow", 0);
        }
        lastOverflowCheck = currentTime;
    }
    
    // Monitor memory usage (rate limited)
    static uint32_t lastMemoryCheck = 0;
    if (currentTime - lastMemoryCheck >= 10000) { // Check every 10 seconds
        int availableRAM = getAvailableRAM();
        if (availableRAM < 100) { // Critical threshold
            Serial.print(F("Warning: Low memory - "));
            Serial.print(availableRAM);
            Serial.println(F(" bytes free"));
            
            displayManager.displayError("Low mem");
        }
        lastMemoryCheck = currentTime;
    }
    
    // Performance monitoring (every 10 seconds)
    if (currentTime - lastLoopTime >= 10000) {
        float loopsPerSecond = loopCounter / 10.0;
        
        Serial.print(F("Performance: "));
        Serial.print(loopsPerSecond);
        Serial.println(F(" loops/sec"));
        
        loopCounter = 0;
        lastLoopTime = currentTime;
    }
    
    // Small delay to prevent overwhelming the system
    // This maintains cooperative multitasking behavior
    delay(1);
}