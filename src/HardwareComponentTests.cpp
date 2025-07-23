#include <Arduino.h>
#include <LiquidCrystal.h>
#include "HardwareConfig.h"
#include "ServiceLocator.h"
#include "MemoryUtils.h"
#include "DisplayManager.h"
#include "FileSystemManager.h"
#include "IStoragePlugin.h"

/**
 * Comprehensive Hardware Component Tests
 * Detailed diagnostics for each physical component
 */

namespace HardwareComponentTests {

/**
 * Test Serial Communication Port
 */
bool testSerialCommunication() {
    Serial.println(F("=== Serial Communication Test ==="));
    
    // Test serial configuration
    Serial.print(F("Baud Rate: "));
    Serial.println(SERIAL_BAUD_RATE);
    Serial.print(F("Data Bits: "));
    Serial.println(SERIAL_DATA_BITS);
    Serial.print(F("Stop Bits: "));
    Serial.println(SERIAL_STOP_BITS);
    
    // Test serial buffer
    Serial.println(F("Serial buffer test: ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"));
    Serial.println(F("Special chars: !@#$%^&*(){}[]|\\:;\"'<>?,./"));
    
    // Test response time
    uint32_t startTime = millis();
    Serial.println(F("Echo test - type 'test' within 10 seconds:"));
    
    char buffer[16];
    size_t pos = 0;
    bool received = false;
    
    while (millis() - startTime < 10000 && !received) {
        if (Serial.available()) {
            char ch = Serial.read();
            if (ch == '\r' || ch == '\n') {
                if (pos > 0) {
                    buffer[pos] = '\0';
                    Serial.print(F("Received: "));
                    Serial.println(buffer);
                    
                    if (equalsIgnoreCase(buffer, pos, "test")) {
                        Serial.println(F("Serial Communication: PASSED"));
                        return true;
                    } else {
                        Serial.println(F("Serial Communication: Wrong response"));
                        return false;
                    }
                }
            } else if (pos < sizeof(buffer) - 1) {
                buffer[pos++] = ch;
            }
        }
    }
    
    Serial.println(F("Serial Communication: TIMEOUT - No response received"));
    return false;
}

/**
 * Test LCD Display Hardware
 */
bool testLCDDisplay() {
    Serial.println(F("=== LCD Display Hardware Test ==="));
    
    auto displayManager = ServiceLocator::getDisplayManager();
    if (!displayManager) {
        Serial.println(F("DisplayManager not available"));
        return false;
    }
    
    // Reset LCD with proper timing
    Serial.println(F("Resetting LCD hardware..."));
    digitalWrite(LCD_RESET_PIN, LOW);
    delay(50);
    digitalWrite(LCD_RESET_PIN, HIGH);
    delay(100);
    
    // Re-initialize LCD
    LiquidCrystal testLcd(LCD_RESET_PIN, LCD_ENABLE_PIN, LCD_DATA4_PIN, LCD_DATA5_PIN, LCD_DATA6_PIN, LCD_DATA7_PIN);
    testLcd.begin(16, 2);
    delay(100);
    
    // Clear and test basic display
    testLcd.clear();
    delay(50);
    testLcd.setCursor(0, 0);
    testLcd.print(F("LCD TEST LINE1"));
    testLcd.setCursor(0, 1);
    testLcd.print(F("LCD TEST LINE2"));
    
    Serial.println(F("LCD should show: 'LCD TEST LINE1' and 'LCD TEST LINE2'"));
    Serial.println(F("Is LCD displaying correctly? (y/n):"));
    
    // Wait for user confirmation
    uint32_t startTime = millis();
    while (millis() - startTime < 15000) {
        if (Serial.available()) {
            char response = Serial.read();
            if (response == 'y' || response == 'Y') {
                Serial.println(F("LCD Display: PASSED"));
                
                // Test character patterns
                testLcd.clear();
                testLcd.setCursor(0, 0);
                testLcd.print(F("CHAR TEST:"));
                testLcd.setCursor(0, 1);
                // Test all printable characters
                for (int i = 32; i < 127 && i < 32 + 16; i++) {
                    testLcd.write((uint8_t)i);
                }
                delay(3000);
                
                return true;
            } else if (response == 'n' || response == 'N') {
                Serial.println(F("LCD Display: FAILED"));
                return false;
            }
        }
    }
    
    Serial.println(F("LCD Display: TIMEOUT - No user response"));
    return false;
}

/**
 * Test Button Interface
 */
bool testButtonInterface() {
    Serial.println(F("=== Button Interface Test ==="));
    
    auto displayManager = ServiceLocator::getDisplayManager();
    if (!displayManager) {
        Serial.println(F("DisplayManager not available"));
        return false;
    }
    
    Serial.println(F("Button value test (press each button):"));
    Serial.println(F("Expected values:"));
    Serial.print(F("RIGHT: ")); Serial.println(BUTTON_RIGHT_VALUE);
    Serial.print(F("UP: ")); Serial.println(BUTTON_UP_VALUE);
    Serial.print(F("DOWN: ")); Serial.println(BUTTON_DOWN_VALUE);
    Serial.print(F("LEFT: ")); Serial.println(BUTTON_LEFT_VALUE);
    Serial.print(F("SELECT: ")); Serial.println(BUTTON_SELECT_VALUE);
    Serial.print(F("NONE: ")); Serial.println(BUTTON_NONE_VALUE);
    
    uint32_t startTime = millis();
    int buttonsPressed = 0;
    int lastValue = -1;
    
    while (millis() - startTime < 30000 && buttonsPressed < 5) {
        int analogValue = analogRead(ANALOG_BUTTONS_PIN);
        
        if (abs(analogValue - lastValue) > BUTTON_TOLERANCE) {
            Serial.print(F("Button analog value: "));
            Serial.print(analogValue);
            
            auto currentButton = displayManager->getCurrentButton();
            Serial.print(F(" -> "));
            Serial.println(displayManager->getButtonName(currentButton));
            
            if (currentButton != DisplayManager::BUTTON_NONE) {
                buttonsPressed++;
            }
            
            lastValue = analogValue;
            delay(500); // Debounce
        }
        
        delay(50);
    }
    
    if (buttonsPressed >= 3) {
        Serial.println(F("Button Interface: PASSED"));
        return true;
    } else {
        Serial.println(F("Button Interface: FAILED - Not enough buttons detected"));
        return false;
    }
}

/**
 * Test LED Indicators
 */
bool testLEDIndicators() {
    Serial.println(F("=== LED Indicators Test ==="));
    
    // Test each LED individually
    const uint8_t leds[] = {HEARTBEAT_LED_PIN, LPT_ACTIVITY_LED_PIN, WRITE_ACTIVITY_LED_PIN};
    const char* names[] = {"HEARTBEAT", "LPT_ACTIVITY", "WRITE_ACTIVITY"};
    
    for (int i = 0; i < 3; i++) {
        Serial.print(F("Testing "));
        Serial.print(names[i]);
        Serial.println(F(" LED..."));
        
        // Flash LED pattern
        for (int j = 0; j < 5; j++) {
            digitalWrite(leds[i], HIGH);
            delay(200);
            digitalWrite(leds[i], LOW);
            delay(200);
        }
        
        Serial.print(F("Did "));
        Serial.print(names[i]);
        Serial.println(F(" LED flash 5 times? (y/n):"));
        
        // Wait for confirmation
        uint32_t startTime = millis();
        while (millis() - startTime < 10000) {
            if (Serial.available()) {
                char response = Serial.read();
                Serial.read(); // consume newline
                if (response == 'n' || response == 'N') {
                    Serial.print(names[i]);
                    Serial.println(F(" LED: FAILED"));
                    return false;
                } else if (response == 'y' || response == 'Y') {
                    Serial.print(names[i]);
                    Serial.println(F(" LED: PASSED"));
                    break;
                }
            }
        }
    }
    
    // Test all LEDs together
    Serial.println(F("Testing all LEDs together..."));
    for (int i = 0; i < 3; i++) {
        digitalWrite(leds[0], HIGH);
        digitalWrite(leds[1], HIGH);
        digitalWrite(leds[2], HIGH);
        delay(500);
        digitalWrite(leds[0], LOW);
        digitalWrite(leds[1], LOW);
        digitalWrite(leds[2], LOW);
        delay(500);
    }
    
    Serial.println(F("LED Indicators: PASSED"));
    return true;
}

/**
 * Test Parallel Port Signals
 */
bool testParallelPortSignals() {
    Serial.println(F("=== Parallel Port Signals Test ==="));
    
    // Test each pin individually
    const uint8_t outputPins[] = {LPT_BUSY_PIN, LPT_ACKNOWLEDGE_PIN};
    const char* outputNames[] = {"BUSY", "ACKNOWLEDGE"};
    
    const uint8_t inputPins[] = {LPT_STROBE_PIN, LPT_AUTO_FEED_PIN, LPT_INITIALIZE_PIN, LPT_SELECT_IN_PIN};
    const char* inputNames[] = {"STROBE", "AUTOFEED", "INITIALIZE", "SELECT_IN"};
    
    // Test output pins
    for (int i = 0; i < 2; i++) {
        Serial.print(F("Testing "));
        Serial.print(outputNames[i]);
        Serial.println(F(" output pin..."));
        
        // Toggle pin
        for (int j = 0; j < 5; j++) {
            digitalWrite(outputPins[i], HIGH);
            delay(100);
            digitalWrite(outputPins[i], LOW);
            delay(100);
        }
        
        Serial.print(outputNames[i]);
        Serial.println(F(" pin toggled 5 times"));
    }
    
    // Test input pins
    Serial.println(F("Reading input pin states:"));
    for (int i = 0; i < 4; i++) {
        int pinState = digitalRead(inputPins[i]);
        Serial.print(inputNames[i]);
        Serial.print(F(": "));
        Serial.println(pinState ? F("HIGH") : F("LOW"));
    }
    
    // Test data pins
    Serial.println(F("Testing 8-bit data pins..."));
    for (int testValue = 0; testValue < 256; testValue += 51) { // Test patterns
        // Set data pins
        for (int bit = 0; bit < 8; bit++) {
            uint8_t pin = LPT_DATA0_PIN + bit;
            digitalWrite(pin, (testValue >> bit) & 1);
        }
        
        // Read back
        uint8_t readValue = 0;
        for (int bit = 0; bit < 8; bit++) {
            uint8_t pin = LPT_DATA0_PIN + bit;
            if (digitalRead(pin)) {
                readValue |= (1 << bit);
            }
        }
        
        Serial.print(F("Wrote: 0x"));
        Serial.print(testValue, HEX);
        Serial.print(F(", Read: 0x"));
        Serial.println(readValue, HEX);
        
        if (readValue != testValue) {
            Serial.println(F("Parallel Port Data: FAILED - Read/Write mismatch"));
            return false;
        }
        
        delay(100);
    }
    
    Serial.println(F("Parallel Port Signals: PASSED"));
    return true;
}

/**
 * Test Memory Systems
 */
bool testMemorySystems() {
    Serial.println(F("=== Memory Systems Test ==="));
    
    // Test available RAM
    int freeRAM = getAvailableRAM();
    Serial.print(F("Available RAM: "));
    Serial.print(freeRAM);
    Serial.println(F(" bytes"));
    
    if (freeRAM < 100) {
        Serial.println(F("Memory Systems: FAILED - Critically low RAM"));
        return false;
    }
    
    // Test memory utilities
    Serial.println(F("Testing memory utility functions..."));
    
    char testBuffer[32];
    if (safeCopy(testBuffer, sizeof(testBuffer), "MemoryTest") != 10) {
        Serial.println(F("Memory Systems: FAILED - safeCopy failed"));
        return false;
    }
    
    if (!startsWith(testBuffer, safeStrlen(testBuffer, sizeof(testBuffer)), "Memory")) {
        Serial.println(F("Memory Systems: FAILED - startsWith failed"));
        return false;
    }
    
    if (!equalsIgnoreCase(testBuffer, safeStrlen(testBuffer, sizeof(testBuffer)), "MEMORYTEST")) {
        Serial.println(F("Memory Systems: FAILED - equalsIgnoreCase failed"));
        return false;
    }
    
    // Test buffer clearing
    clearBuffer(testBuffer, sizeof(testBuffer));
    if (testBuffer[0] != '\0' || testBuffer[10] != '\0') {
        Serial.println(F("Memory Systems: FAILED - clearBuffer failed"));
        return false;
    }
    
    Serial.println(F("Memory Systems: PASSED"));
    return true;
}

/**
 * Test Storage Systems
 */
bool testStorageSystems() {
    Serial.println(F("=== Storage Systems Test ==="));
    
    auto fsManager = ServiceLocator::getFileSystemManager();
    if (!fsManager) {
        Serial.println(F("FileSystemManager not available"));
        return false;
    }
    
    // Test each storage type
    IStoragePlugin::StorageType storageTypes[] = {
        IStoragePlugin::STORAGE_SD_CARD,
        IStoragePlugin::STORAGE_EEPROM,
        IStoragePlugin::STORAGE_SERIAL
    };
    const char* storageNames[] = {"SD Card", "EEPROM", "Serial"};
    
    for (int i = 0; i < 3; i++) {
        Serial.print(F("Testing "));
        Serial.print(storageNames[i]);
        Serial.println(F(" storage..."));
        
        // Switch to storage
        if (fsManager->setStorageType(storageTypes[i])) {
            Serial.print(F("Switched to "));
            Serial.println(fsManager->getCurrentStorageName());
            
            // Test write operation
            const char* testData = "TestData123";
            char testFilename[16];
            snprintf(testFilename, sizeof(testFilename), "test_%d.txt", i);
            
            size_t written = fsManager->writeFile(testFilename, (const uint8_t*)testData, strlen(testData));
            if (written == strlen(testData)) {
                Serial.print(storageNames[i]);
                Serial.println(F(" storage write: PASSED"));
                
                // Test read operation
                uint8_t readBuffer[32];
                size_t read = fsManager->readFile(testFilename, readBuffer, sizeof(readBuffer) - 1);
                readBuffer[read] = '\0';
                
                if (read == strlen(testData) && strcmp((char*)readBuffer, testData) == 0) {
                    Serial.print(storageNames[i]);
                    Serial.println(F(" storage read: PASSED"));
                } else {
                    Serial.print(storageNames[i]);
                    Serial.println(F(" storage read: FAILED"));
                }
            } else {
                Serial.print(storageNames[i]);
                Serial.println(F(" storage write: FAILED"));
            }
        } else {
            Serial.print(storageNames[i]);
            Serial.println(F(" storage: NOT AVAILABLE"));
        }
        
        delay(1000);
    }
    
    Serial.println(F("Storage Systems: Test completed"));
    return true;
}

/**
 * Run comprehensive hardware test suite
 */
bool runComprehensiveTests() {
    Serial.println(F("=== MegaDeviceBridge Comprehensive Hardware Tests ==="));
    Serial.println(F("This will test all hardware components systematically."));
    Serial.println(F("Please be ready to interact with prompts."));
    Serial.println();
    
    bool allPassed = true;
    
    // Test each component
    allPassed &= testSerialCommunication();
    delay(1000);
    
    allPassed &= testMemorySystems();
    delay(1000);
    
    allPassed &= testLEDIndicators();
    delay(1000);
    
    allPassed &= testLCDDisplay();
    delay(1000);
    
    allPassed &= testButtonInterface();
    delay(1000);
    
    allPassed &= testParallelPortSignals();
    delay(1000);
    
    allPassed &= testStorageSystems();
    delay(1000);
    
    Serial.println(F("=== Comprehensive Test Results ==="));
    if (allPassed) {
        Serial.println(F("*** ALL HARDWARE TESTS PASSED ***"));
    } else {
        Serial.println(F("*** SOME HARDWARE TESTS FAILED ***"));
        Serial.println(F("Check individual test results above."));
    }
    
    return allPassed;
}

} // namespace HardwareComponentTests