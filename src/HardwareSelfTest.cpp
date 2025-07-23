#include <Arduino.h>
#include "HardwareConfig.h"
#include "ServiceLocator.h"
#include "MemoryUtils.h"
#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "HeartbeatLEDManager.h"

/**
 * Hardware Self-Test Implementation
 * Comprehensive validation of all hardware components
 */

namespace HardwareSelfTest {

struct TestResult {
    bool passed;
    char description[32];
    int errorCode;
};

/**
 * Test parallel port pins and signals
 */
TestResult testParallelPort() {
    TestResult result = {true, "Parallel Port", 0};
    
    auto parallelManager = ServiceLocator::getParallelPortManager();
    if (!parallelManager) {
        result.passed = false;
        result.errorCode = 1;
        safeCopy(result.description, sizeof(result.description), "PPM not found");
        return result;
    }
    
    // Test protocol signals
    if (!parallelManager->testProtocolSignals()) {
        result.passed = false;
        result.errorCode = 2;
        safeCopy(result.description, sizeof(result.description), "Protocol test failed");
        return result;
    }
    
    // Test interrupt for 2 seconds
    uint32_t intCount = parallelManager->testInterrupt(2000);
    Serial.print(F("Parallel port interrupts in 2s: "));
    Serial.println(intCount);
    
    safeCopy(result.description, sizeof(result.description), "Parallel Port OK");
    return result;
}

/**
 * Test storage systems
 */
TestResult testStorage() {
    TestResult result = {true, "Storage Systems", 0};
    
    auto fsManager = ServiceLocator::getFileSystemManager();
    if (!fsManager) {
        result.passed = false;
        result.errorCode = 1;
        safeCopy(result.description, sizeof(result.description), "FSM not found");
        return result;
    }
    
    // Test write operation
    if (!fsManager->testWrite()) {
        result.passed = false;
        result.errorCode = 2;
        safeCopy(result.description, sizeof(result.description), "Write test failed");
        return result;
    }
    
    // Validate all storages
    if (!fsManager->validateAllStorages()) {
        result.passed = false;
        result.errorCode = 3;
        safeCopy(result.description, sizeof(result.description), "Storage validation failed");
        return result;
    }
    
    safeCopy(result.description, sizeof(result.description), "Storage OK");
    return result;
}

/**
 * Test LCD and button interface
 */
TestResult testDisplay() {
    TestResult result = {true, "Display & Buttons", 0};
    
    auto displayManager = ServiceLocator::getDisplayManager();
    if (!displayManager) {
        result.passed = false;
        result.errorCode = 1;
        safeCopy(result.description, sizeof(result.description), "DM not found");
        return result;
    }
    
    // Display test message
    displayManager->displayMessagePGM(F("Self-Test"), F("Display OK"), 2000);
    
    // Test progress bar
    for (int i = 0; i <= 100; i += 20) {
        displayManager->displayProgressBar(i, 1, "Progress");
        delay(500);
    }
    
    safeCopy(result.description, sizeof(result.description), "Display OK");
    return result;
}

/**
 * Test LEDs
 */
TestResult testLEDs() {
    TestResult result = {true, "LED Indicators", 0};
    
    // Test all LEDs
    digitalWrite(HEARTBEAT_LED_PIN, HIGH);
    digitalWrite(LPT_ACTIVITY_LED_PIN, HIGH);
    digitalWrite(WRITE_ACTIVITY_LED_PIN, HIGH);
    delay(1000);
    
    digitalWrite(HEARTBEAT_LED_PIN, LOW);
    digitalWrite(LPT_ACTIVITY_LED_PIN, LOW);
    digitalWrite(WRITE_ACTIVITY_LED_PIN, LOW);
    delay(500);
    
    // Flash pattern
    for (int i = 0; i < 3; i++) {
        digitalWrite(HEARTBEAT_LED_PIN, HIGH);
        delay(200);
        digitalWrite(LPT_ACTIVITY_LED_PIN, HIGH);
        delay(200);
        digitalWrite(WRITE_ACTIVITY_LED_PIN, HIGH);
        delay(200);
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
        digitalWrite(LPT_ACTIVITY_LED_PIN, LOW);
        digitalWrite(WRITE_ACTIVITY_LED_PIN, LOW);
        delay(200);
    }
    
    safeCopy(result.description, sizeof(result.description), "LEDs OK");
    return result;
}

/**
 * Test memory systems
 */
TestResult testMemory() {
    TestResult result = {true, "Memory Systems", 0};
    
    // Check available RAM
    int freeRAM = getAvailableRAM();
    if (freeRAM < 100) { // Minimum 100 bytes free
        result.passed = false;
        result.errorCode = 1;
        safeCopy(result.description, sizeof(result.description), "Low RAM");
        return result;
    }
    
    Serial.print(F("Available RAM: "));
    Serial.print(freeRAM);
    Serial.println(F(" bytes"));
    
    // Test memory utilities
    char testBuffer[32];
    if (safeCopy(testBuffer, sizeof(testBuffer), "MemTest") != 7) {
        result.passed = false;
        result.errorCode = 2;
        safeCopy(result.description, sizeof(result.description), "Memory utils failed");
        return result;
    }
    
    safeCopy(result.description, sizeof(result.description), "Memory OK");
    return result;
}

/**
 * Test system components
 */
TestResult testSystemComponents() {
    TestResult result = {true, "System Components", 0};
    
    // Validate all components
    if (!ServiceLocator::validateAll()) {
        result.passed = false;
        result.errorCode = 1;
        safeCopy(result.description, sizeof(result.description), "Component validation failed");
        return result;
    }
    
    // Get component memory usage
    size_t totalMemory = ServiceLocator::getTotalMemoryUsage();
    Serial.print(F("Component memory usage: "));
    Serial.print(totalMemory);
    Serial.println(F(" bytes"));
    
    safeCopy(result.description, sizeof(result.description), "Components OK");
    return result;
}

/**
 * Run complete hardware self-test
 */
bool runCompleteSelfTest() {
    Serial.println(F("=== MegaDeviceBridge Hardware Self-Test ==="));
    
    TestResult tests[] = {
        testMemory(),
        testSystemComponents(),
        testLEDs(),
        testDisplay(),
        testStorage(),
        testParallelPort()
    };
    
    const size_t numTests = sizeof(tests) / sizeof(tests[0]);
    size_t passed = 0;
    
    auto displayManager = ServiceLocator::getDisplayManager();
    
    for (size_t i = 0; i < numTests; i++) {
        Serial.print(F("Test "));
        Serial.print(i + 1);
        Serial.print(F("/"));
        Serial.print(numTests);
        Serial.print(F(": "));
        Serial.print(tests[i].description);
        Serial.print(F(" - "));
        
        if (tests[i].passed) {
            Serial.println(F("PASSED"));
            passed++;
        } else {
            Serial.print(F("FAILED ("));
            Serial.print(tests[i].errorCode);
            Serial.println(F(")"));
            
            if (displayManager) {
                displayManager->displayError(tests[i].description, tests[i].errorCode);
                delay(2000);
            }
        }
    }
    
    Serial.println(F("=== Self-Test Results ==="));
    Serial.print(F("Passed: "));
    Serial.print(passed);
    Serial.print(F("/"));
    Serial.println(numTests);
    
    bool allPassed = (passed == numTests);
    
    if (displayManager) {
        if (allPassed) {
            displayManager->displayMessagePGM(F("Self-Test"), F("ALL PASSED"), 3000);
        } else {
            char failMsg[17];
            snprintf(failMsg, sizeof(failMsg), "%d/%d FAILED", (int)(numTests - passed), (int)numTests);
            displayManager->displayMessage("Self-Test", failMsg, 3000);
        }
    }
    
    if (allPassed) {
        Serial.println(F("*** ALL TESTS PASSED - HARDWARE OK ***"));
        
        // Success LED pattern
        auto heartbeatManager = ServiceLocator::getHeartbeatLEDManager();
        if (heartbeatManager) {
            for (int i = 0; i < 5; i++) {
                digitalWrite(HEARTBEAT_LED_PIN, HIGH);
                delay(100);
                digitalWrite(HEARTBEAT_LED_PIN, LOW);
                delay(100);
            }
        }
    } else {
        Serial.println(F("*** SOME TESTS FAILED - CHECK HARDWARE ***"));
        
        // Failure SOS pattern
        auto heartbeatManager = ServiceLocator::getHeartbeatLEDManager();
        if (heartbeatManager) {
            heartbeatManager->triggerSOSPattern();
        }
    }
    
    return allPassed;
}

/**
 * Quick system health check
 */
bool quickHealthCheck() {
    // Fast validation of critical components
    if (!ServiceLocator::allComponentsRegistered()) {
        Serial.println(F("Health Check: Components not registered"));
        return false;
    }
    
    if (!ServiceLocator::validateAll()) {
        Serial.println(F("Health Check: Component validation failed"));
        return false;
    }
    
    int freeRAM = getAvailableRAM();
    if (freeRAM < 50) {
        Serial.print(F("Health Check: Critical low memory - "));
        Serial.print(freeRAM);
        Serial.println(F(" bytes"));
        return false;
    }
    
    Serial.println(F("Health Check: PASSED"));
    return true;
}

} // namespace HardwareSelfTest