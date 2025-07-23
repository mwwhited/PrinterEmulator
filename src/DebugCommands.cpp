#include <Arduino.h>
#include "HardwareConfig.h"
#include "ServiceLocator.h"
#include "MemoryUtils.h"
#include "HardwareSelfTest.h"
#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "HeartbeatLEDManager.h"

/**
 * Debug Commands Implementation
 * Comprehensive serial command interface for system monitoring and control
 */

namespace DebugCommands {

// Command buffer for parsing
static char commandBuffer[COMMAND_BUFFER_SIZE];
static bool commandReady = false;

/**
 * Process serial input and build command buffer
 */
void processSerialInput() {
    static size_t commandPos = 0;
    
    while (Serial.available() > 0) {
        char ch = Serial.read();
        
        if (ch == '\r' || ch == '\n') {
            if (commandPos > 0) {
                commandBuffer[commandPos] = '\0';
                commandReady = true;
            }
        } else if (commandPos < COMMAND_BUFFER_SIZE - 1) {
            commandBuffer[commandPos++] = ch;
        } else {
            // Buffer overflow, reset
            commandPos = 0;
            Serial.println(F("ERROR: Command too long"));
        }
    }
}

/**
 * Display help menu
 */
void showHelp() {
    Serial.println(F("=== MegaDeviceBridge Debug Commands ==="));
    Serial.println(F("System Commands:"));
    Serial.println(F("  help          - Show this help"));
    Serial.println(F("  info          - System information"));
    Serial.println(F("  status        - Component status"));
    Serial.println(F("  validate      - Validate all components"));
    Serial.println(F("  restart       - Software restart"));
    Serial.println(F("  selftest      - Run hardware self-test"));
    Serial.println();
    Serial.println(F("Storage Commands:"));
    Serial.println(F("  storage       - Show storage status"));
    Serial.println(F("  storage sd    - Switch to SD card"));
    Serial.println(F("  storage eeprom - Switch to EEPROM"));
    Serial.println(F("  storage serial - Switch to Serial"));
    Serial.println(F("  list          - List files"));
    Serial.println(F("  testwrite     - Test file write"));
    Serial.println();
    Serial.println(F("Hardware Commands:"));
    Serial.println(F("  parallel      - Parallel port status"));
    Serial.println(F("  testint       - Test interrupts (10s)"));
    Serial.println(F("  testlpt       - Test LPT signals"));
    Serial.println(F("  buttons       - Show button values"));
    Serial.println(F("  led on/off    - Control LEDs"));
    Serial.println();
    Serial.println(F("Debug Commands:"));
    Serial.println(F("  debug on/off  - Enable/disable debug"));
    Serial.println(F("  memory        - Memory usage info"));
    Serial.println(F("  reset         - Reset components"));
    Serial.println();
}

/**
 * Show system information
 */
void showSystemInfo() {
    Serial.println(F("=== System Information ==="));
    Serial.println(F("Device: MegaDeviceBridge v1.0"));
    Serial.println(F("MCU: Arduino Mega 2560 (ATmega2560)"));
    Serial.print(F("Clock: "));
    Serial.print(F_CPU / 1000000);
    Serial.println(F(" MHz"));
    Serial.print(F("Uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" seconds"));
    
    int freeRAM = getAvailableRAM();
    Serial.print(F("Free RAM: "));
    Serial.print(freeRAM);
    Serial.print(F("/8192 bytes ("));
    Serial.print((freeRAM * 100) / 8192);
    Serial.println(F("%)"));
    
    size_t componentMemory = ServiceLocator::getTotalMemoryUsage();
    Serial.print(F("Component Memory: "));
    Serial.print(componentMemory);
    Serial.println(F(" bytes"));
    
    Serial.println();
}

/**
 * Show component status
 */
void showComponentStatus() {
    Serial.println(F("=== Component Status ==="));
    
    size_t componentCount;
    IComponent** components = ServiceLocator::getAllComponents(componentCount);
    
    for (size_t i = 0; i < componentCount; i++) {
        if (components[i]) {
            Serial.print(components[i]->getName());
            Serial.print(F(": "));
            
            int status = components[i]->getStatus();
            switch (status) {
                case STATUS_OK:
                    Serial.print(F("OK"));
                    break;
                case STATUS_ERROR:
                    Serial.print(F("ERROR"));
                    break;
                case STATUS_NOT_INITIALIZED:
                    Serial.print(F("NOT_INIT"));
                    break;
                case STATUS_BUSY:
                    Serial.print(F("BUSY"));
                    break;
                default:
                    Serial.print(F("UNKNOWN("));
                    Serial.print(status);
                    Serial.print(F(")"));
                    break;
            }
            
            Serial.print(F(" ("));
            Serial.print(components[i]->getMemoryUsage());
            Serial.println(F(" bytes)"));
        }
    }
    Serial.println();
}

/**
 * Show parallel port status
 */
void showParallelPortStatus() {
    auto parallelManager = ServiceLocator::getParallelPortManager();
    if (!parallelManager) {
        Serial.println(F("ParallelPortManager not available"));
        return;
    }
    
    Serial.println(F("=== Parallel Port Status ==="));
    Serial.print(F("Capture Enabled: "));
    Serial.println(parallelManager->isCaptureEnabled() ? F("YES") : F("NO"));
    
    Serial.print(F("Available Bytes: "));
    Serial.println(parallelManager->getAvailableBytes());
    
    Serial.print(F("Buffer Utilization: "));
    Serial.print(parallelManager->getBufferUtilization());
    Serial.println(F("%"));
    
    Serial.print(F("Total Bytes Received: "));
    Serial.println(parallelManager->getTotalBytesReceived());
    
    Serial.print(F("Overflow Count: "));
    Serial.println(parallelManager->getOverflowCount());
    
    uint32_t totalInts;
    uint16_t maxTime, avgTime;
    parallelManager->getInterruptStats(totalInts, maxTime, avgTime);
    Serial.print(F("Total Interrupts: "));
    Serial.println(totalInts);
    Serial.print(F("Max ISR Time: "));
    Serial.print(maxTime);
    Serial.println(F(" μs"));
    Serial.print(F("Avg ISR Time: "));
    Serial.print(avgTime);
    Serial.println(F(" μs"));
    
    bool busy, ack, error;
    parallelManager->getPortStatus(busy, ack, error);
    Serial.print(F("Port Status - Busy: "));
    Serial.print(busy ? F("HIGH") : F("LOW"));
    Serial.print(F(", Ack: "));
    Serial.print(ack ? F("LOW") : F("HIGH"));
    Serial.print(F(", Error: "));
    Serial.println(error ? F("LOW") : F("HIGH"));
    
    Serial.println();
}

/**
 * Show storage status
 */
void showStorageStatus() {
    auto fsManager = ServiceLocator::getFileSystemManager();
    if (!fsManager) {
        Serial.println(F("FileSystemManager not available"));
        return;
    }
    
    Serial.println(F("=== Storage Status ==="));
    Serial.print(F("Current Storage: "));
    Serial.println(fsManager->getCurrentStorageName());
    
    Serial.print(F("Storage Ready: "));
    Serial.println(fsManager->isStorageReady() ? F("YES") : F("NO"));
    
    uint32_t available, total;
    if (fsManager->getStorageSpace(available, total)) {
        Serial.print(F("Available Space: "));
        Serial.print(available / 1024);
        Serial.println(F(" KB"));
        Serial.print(F("Total Space: "));
        Serial.print(total / 1024);
        Serial.println(F(" KB"));
    }
    
    uint32_t filesWritten, bytesWritten, filesRead, bytesRead;
    fsManager->getStatistics(filesWritten, bytesWritten, filesRead, bytesRead);
    Serial.print(F("Files Written: "));
    Serial.println(filesWritten);
    Serial.print(F("Bytes Written: "));
    Serial.println(bytesWritten);
    Serial.print(F("Files Read: "));
    Serial.println(filesRead);
    Serial.print(F("Bytes Read: "));
    Serial.println(bytesRead);
    
    Serial.println();
}

/**
 * Control LEDs
 */
void controlLED(const char* command) {
    if (startsWith(command, 32, "on")) {
        digitalWrite(HEARTBEAT_LED_PIN, HIGH);
        digitalWrite(LPT_ACTIVITY_LED_PIN, HIGH);
        digitalWrite(WRITE_ACTIVITY_LED_PIN, HIGH);
        Serial.println(F("LEDs ON"));
    } else if (startsWith(command, 32, "off")) {
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
        digitalWrite(LPT_ACTIVITY_LED_PIN, LOW);
        digitalWrite(WRITE_ACTIVITY_LED_PIN, LOW);
        Serial.println(F("LEDs OFF"));
    } else {
        Serial.println(F("Usage: led on|off"));
    }
}

/**
 * Show button values
 */
void showButtonValues() {
    auto displayManager = ServiceLocator::getDisplayManager();
    if (!displayManager) {
        Serial.println(F("DisplayManager not available"));
        return;
    }
    
    Serial.println(F("=== Button Values ==="));
    Serial.println(F("Press buttons to see values (10 seconds):"));
    
    uint32_t startTime = millis();
    while (millis() - startTime < 10000) {
        int analogValue = analogRead(ANALOG_BUTTONS_PIN);
        auto currentButton = displayManager->getCurrentButton();
        
        Serial.print(F("Analog: "));
        Serial.print(analogValue);
        Serial.print(F(", Button: "));
        Serial.println(displayManager->getButtonName(currentButton));
        
        delay(200);
    }
    Serial.println();
}

/**
 * Process a single command
 */
void processCommand(const char* cmd) {
    if (!cmd || safeStrlen(cmd, COMMAND_BUFFER_SIZE) == 0) {
        return;
    }
    
    if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "help")) {
        showHelp();
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "info")) {
        showSystemInfo();
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "status")) {
        showComponentStatus();
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "validate")) {
        bool valid = ServiceLocator::validateAll();
        Serial.print(F("Component Validation: "));
        Serial.println(valid ? F("PASSED") : F("FAILED"));
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "restart")) {
        Serial.println(F("Restarting system..."));
        delay(1000);
        asm volatile ("  jmp 0");
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "selftest")) {
        HardwareSelfTest::runCompleteSelfTest();
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "storage")) {
        showStorageStatus();
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "parallel")) {
        showParallelPortStatus();
    } else if (startsWith(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "testint")) {
        auto parallelManager = ServiceLocator::getParallelPortManager();
        if (parallelManager) {
            parallelManager->testInterrupt(10000);
        }
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "testlpt")) {
        auto parallelManager = ServiceLocator::getParallelPortManager();
        if (parallelManager) {
            parallelManager->testProtocolSignals();
        }
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "buttons")) {
        showButtonValues();
    } else if (startsWith(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "led ")) {
        controlLED(cmd + 4);
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "memory")) {
        Serial.print(F("Available RAM: "));
        Serial.print(getAvailableRAM());
        Serial.println(F(" bytes"));
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "testwrite")) {
        auto fsManager = ServiceLocator::getFileSystemManager();
        if (fsManager) {
            bool result = fsManager->testWrite();
            Serial.print(F("Test Write: "));
            Serial.println(result ? F("PASSED") : F("FAILED"));
        }
    } else if (equalsIgnoreCase(cmd, safeStrlen(cmd, COMMAND_BUFFER_SIZE), "reset")) {
        Serial.println(F("Resetting all components..."));
        int result = ServiceLocator::resetAll();
        Serial.print(F("Reset Result: "));
        Serial.println(result == STATUS_OK ? F("OK") : F("FAILED"));
    } else {
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        Serial.println(F("Type 'help' for available commands"));
    }
}

/**
 * Main update function - call from main loop
 */
void update() {
    processSerialInput();
    
    if (commandReady) {
        processCommand(commandBuffer);
        commandReady = false;
        
        // Reset command buffer
        clearBuffer(commandBuffer, sizeof(commandBuffer));
    }
}

/**
 * Initialize debug command system
 */
void initialize() {
    clearBuffer(commandBuffer, sizeof(commandBuffer));
    commandReady = false;
    
    Serial.println(F("Debug command system initialized"));
    Serial.println(F("Type 'help' for available commands"));
}

} // namespace DebugCommands