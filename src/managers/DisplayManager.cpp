#include "DisplayManager.h"
#include "MemoryUtils.h"
#include "ServiceLocator.h"

// Custom characters for progress bar
uint8_t progressBarChars[8][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Empty
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}, // 1/5
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, // 2/5
    {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C}, // 3/5
    {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E}, // 4/5
    {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, // Full
    {0x00, 0x1F, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x1F}, // Error pattern
    {0x1F, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x1F, 0x00}  // Warning pattern
};

DisplayManager::DisplayManager() 
    : lcd(LCD_RESET_PIN, LCD_ENABLE_PIN, LCD_DATA4_PIN, LCD_DATA5_PIN, LCD_DATA6_PIN, LCD_DATA7_PIN),
      initialized(false), debugEnabled(false), currentState(STATE_IDLE),
      lastButton(BUTTON_NONE), currentButton(BUTTON_NONE),
      buttonPressTime(0), buttonReleaseTime(0), buttonHeld(false),
      displayUpdateTime(0), messageTimeout(0), messageTimedOut(false),
      currentMenuItem(0), totalMenuItems(0),
      scrollPosition(0), scrollUpdateTime(0), scrollEnabled(false),
      statusUpdateTime(0), autoStatusUpdate(false), backlightEnabled(true) {
    
    clearBuffer(line1Buffer, sizeof(line1Buffer));
    clearBuffer(line2Buffer, sizeof(line2Buffer));
    clearBuffer(messageBuffer, sizeof(messageBuffer));
    clearBuffer(menuItems, sizeof(menuItems));
}

int DisplayManager::initialize() {
    if (initialized) {
        return STATUS_OK;
    }
    
    // Initialize LCD
    lcd.begin(16, 2);
    
    // Setup custom characters for progress bar
    setupProgressBarChars();
    
    // Clear display
    clearDisplay();
    
    // Display startup message
    displayMessagePGM(F("MegaDeviceBridge"), F("Initializing..."), 2000);
    
    initialized = true;
    
    if (debugEnabled) {
        Serial.println(F("DisplayManager: Initialized"));
    }
    
    return STATUS_OK;
}

int DisplayManager::update() {
    if (!initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    uint32_t currentTime = millis();
    
    // Update button state
    updateButtonState();
    
    // Handle message timeout
    if (currentState == STATE_MESSAGE && messageTimeout > 0) {
        if (currentTime >= messageTimeout) {
            currentState = STATE_IDLE;
            messageTimedOut = true;
            clearDisplay();
        }
    }
    
    // Update scrolling text
    if (currentState == STATE_SCROLLING && scrollEnabled) {
        updateScrolling();
    }
    
    // Auto status update
    if (autoStatusUpdate && currentState == STATE_IDLE) {
        if (currentTime - statusUpdateTime >= 2000) { // Update every 2 seconds
            // Get system status from SystemManager
            auto systemManager = ServiceLocator::getSystemManager();
            if (systemManager) {
                char statusLine1[17];
                char statusLine2[17];
                
                // Simple status display
                safeCopy(statusLine1, sizeof(statusLine1), "System: Ready");
                
                // Display available RAM
                int freeRAM = getAvailableRAM();
                snprintf(statusLine2, sizeof(statusLine2), "RAM: %dB free", freeRAM);
                
                displayStatus(statusLine1, statusLine2);
            }
            statusUpdateTime = currentTime;
        }
    }
    
    // Update display content
    updateDisplay();
    
    return STATUS_OK;
}

int DisplayManager::getStatus() const {
    if (!initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    return STATUS_OK;
}

const __FlashStringHelper* DisplayManager::getName() const {
    return F("DisplayManager");
}

bool DisplayManager::validate() const {
    return initialized;
}

int DisplayManager::reset() {
    if (initialized) {
        clearDisplay();
        currentState = STATE_IDLE;
        messageTimeout = 0;
        messageTimedOut = false;
        scrollEnabled = false;
        autoStatusUpdate = false;
    }
    
    return initialize();
}

size_t DisplayManager::getMemoryUsage() const {
    return sizeof(*this);
}

void DisplayManager::setDebugEnabled(bool enabled) {
    debugEnabled = enabled;
}

bool DisplayManager::isDebugEnabled() const {
    return debugEnabled;
}

DisplayManager::ButtonType DisplayManager::readButton() {
    int analogValue = analogRead(ANALOG_BUTTONS_PIN);
    
    // Determine button based on analog value with tolerance
    if (analogValue < BUTTON_RIGHT_VALUE + BUTTON_TOLERANCE) {
        return BUTTON_RIGHT;
    } else if (analogValue < BUTTON_UP_VALUE + BUTTON_TOLERANCE) {
        return BUTTON_UP;
    } else if (analogValue < BUTTON_DOWN_VALUE + BUTTON_TOLERANCE) {
        return BUTTON_DOWN;
    } else if (analogValue < BUTTON_LEFT_VALUE + BUTTON_TOLERANCE) {
        return BUTTON_LEFT;
    } else if (analogValue < BUTTON_SELECT_VALUE + BUTTON_TOLERANCE) {
        return BUTTON_SELECT;
    } else {
        return BUTTON_NONE;
    }
}

void DisplayManager::updateButtonState() {
    ButtonType newButton = readButton();
    uint32_t currentTime = millis();
    
    if (newButton != currentButton) {
        if (currentButton == BUTTON_NONE && newButton != BUTTON_NONE) {
            // Button pressed
            buttonPressTime = currentTime;
            buttonHeld = false;
            handleButtonPress(newButton);
        } else if (currentButton != BUTTON_NONE && newButton == BUTTON_NONE) {
            // Button released
            buttonReleaseTime = currentTime;
            buttonHeld = false;
        }
        
        lastButton = currentButton;
        currentButton = newButton;
    } else if (currentButton != BUTTON_NONE) {
        // Check for held button
        if (!buttonHeld && (currentTime - buttonPressTime) > 1000) { // 1 second hold
            buttonHeld = true;
            // Handle held button action
            if (debugEnabled) {
                Serial.println(F("DisplayManager: Button held"));
            }
        }
    }
}

void DisplayManager::clearDisplay() {
    lcd.clear();
    clearBuffer(line1Buffer, sizeof(line1Buffer));
    clearBuffer(line2Buffer, sizeof(line2Buffer));
    clearBuffer(messageBuffer, sizeof(messageBuffer));
}

void DisplayManager::updateDisplay() {
    uint32_t currentTime = millis();
    
    // Throttle updates to prevent flicker
    if (currentTime - displayUpdateTime < 100) { // 100ms minimum between updates
        return;
    }
    
    switch (currentState) {
        case STATE_IDLE:
            // Display default content
            break;
            
        case STATE_MESSAGE:
            // Message is already displayed, just check timeout
            break;
            
        case STATE_STATUS:
            // Status display - could add blinking indicators here
            break;
            
        case STATE_SCROLLING:
            // Scrolling is handled in updateScrolling()
            break;
            
        case STATE_MENU:
            // Menu display is handled in showMenu()
            break;
    }
    
    displayUpdateTime = currentTime;
}

void DisplayManager::handleButtonPress(ButtonType button) {
    if (debugEnabled) {
        Serial.print(F("DisplayManager: Button pressed: "));
        Serial.println((int)button);
    }
    
    switch (currentState) {
        case STATE_MESSAGE:
            // Any button press clears message
            currentState = STATE_IDLE;
            clearDisplay();
            break;
            
        case STATE_MENU:
            // Menu navigation is handled by showMenu()
            break;
            
        case STATE_IDLE:
        case STATE_STATUS:
            // Button actions in idle/status state
            switch (button) {
                case BUTTON_SELECT:
                    // Toggle between idle and status
                    if (currentState == STATE_IDLE) {
                        currentState = STATE_STATUS;
                    } else {
                        currentState = STATE_IDLE;
                        clearDisplay();
                    }
                    break;
                    
                default:
                    // Other buttons could trigger specific actions
                    break;
            }
            break;
            
        default:
            break;
    }
}

void DisplayManager::updateScrolling() {
    uint32_t currentTime = millis();
    
    if (currentTime - scrollUpdateTime >= 300) { // Scroll every 300ms
        size_t messageLen = safeStrlen(messageBuffer, sizeof(messageBuffer));
        
        if (messageLen > 16) {
            // Extract 16 characters starting from scrollPosition
            char displayText[17];
            clearBuffer(displayText, sizeof(displayText));
            
            for (size_t i = 0; i < 16 && (scrollPosition + i) < messageLen; i++) {
                displayText[i] = messageBuffer[scrollPosition + i];
            }
            
            // Display on line 1
            lcd.setCursor(0, 0);
            lcd.print(displayText);
            
            scrollPosition++;
            if (scrollPosition >= messageLen - 16) {
                scrollPosition = 0; // Restart scrolling
            }
        }
        
        scrollUpdateTime = currentTime;
    }
}

void DisplayManager::formatStatusLine(const char* statusText, char* buffer, size_t bufferSize) {
    if (!statusText || !buffer || bufferSize == 0) {
        return;
    }
    
    truncateText(statusText, buffer, bufferSize);
}

void DisplayManager::centerText(const char* text, char* buffer, size_t bufferSize) {
    if (!text || !buffer || bufferSize < 17) {
        return;
    }
    
    size_t textLen = safeStrlen(text, 16);
    size_t padding = (16 - textLen) / 2;
    
    clearBuffer(buffer, bufferSize);
    
    // Add padding spaces
    for (size_t i = 0; i < padding; i++) {
        buffer[i] = ' ';
    }
    
    // Copy text
    safeCopy(buffer + padding, bufferSize - padding, text, textLen);
}

void DisplayManager::truncateText(const char* text, char* buffer, size_t bufferSize) {
    if (!text || !buffer || bufferSize == 0) {
        return;
    }
    
    safeCopy(buffer, bufferSize, text, 16);
}

void DisplayManager::displayMessage(const char* line1, const char* line2, uint32_t timeoutMs) {
    if (!initialized) {
        return;
    }
    
    clearDisplay();
    
    if (line1) {
        truncateText(line1, line1Buffer, sizeof(line1Buffer));
        lcd.setCursor(0, 0);
        lcd.print(line1Buffer);
    }
    
    if (line2) {
        truncateText(line2, line2Buffer, sizeof(line2Buffer));
        lcd.setCursor(0, 1);
        lcd.print(line2Buffer);
    }
    
    currentState = STATE_MESSAGE;
    
    if (timeoutMs > 0) {
        messageTimeout = millis() + timeoutMs;
    } else {
        messageTimeout = 0;
    }
    
    if (debugEnabled) {
        Serial.print(F("DisplayManager: Message displayed: "));
        if (line1) Serial.print(line1);
        if (line2) {
            Serial.print(F(" / "));
            Serial.print(line2);
        }
        Serial.println();
    }
}

void DisplayManager::displayMessagePGM(const __FlashStringHelper* line1, 
                                      const __FlashStringHelper* line2, 
                                      uint32_t timeoutMs) {
    if (!initialized) {
        return;
    }
    
    clearDisplay();
    
    if (line1) {
        safeCopyPGM(line1Buffer, sizeof(line1Buffer), line1);
        lcd.setCursor(0, 0);
        lcd.print(line1Buffer);
    }
    
    if (line2) {
        safeCopyPGM(line2Buffer, sizeof(line2Buffer), line2);
        lcd.setCursor(0, 1);
        lcd.print(line2Buffer);
    }
    
    currentState = STATE_MESSAGE;
    
    if (timeoutMs > 0) {
        messageTimeout = millis() + timeoutMs;
    } else {
        messageTimeout = 0;
    }
}

void DisplayManager::displayScrollingMessage(const char* message, uint8_t line, uint32_t scrollSpeed) {
    if (!initialized || !message) {
        return;
    }
    
    safeCopy(messageBuffer, sizeof(messageBuffer), message);
    scrollPosition = 0;
    scrollUpdateTime = millis();
    scrollEnabled = true;
    currentState = STATE_SCROLLING;
    
    // Clear the other line
    lcd.setCursor(0, line == 0 ? 1 : 0);
    lcd.print(F("                ")); // 16 spaces
}

void DisplayManager::displayStatus(const char* statusLine1, const char* statusLine2) {
    if (!initialized) {
        return;
    }
    
    clearDisplay();
    
    if (statusLine1) {
        formatStatusLine(statusLine1, line1Buffer, sizeof(line1Buffer));
        lcd.setCursor(0, 0);
        lcd.print(line1Buffer);
    }
    
    if (statusLine2) {
        formatStatusLine(statusLine2, line2Buffer, sizeof(line2Buffer));
        lcd.setCursor(0, 1);
        lcd.print(line2Buffer);
    }
    
    currentState = STATE_STATUS;
}

void DisplayManager::clearAndIdle() {
    clearDisplay();
    currentState = STATE_IDLE;
    messageTimeout = 0;
    scrollEnabled = false;
}

void DisplayManager::setAutoStatusUpdate(bool enabled, uint32_t updateInterval) {
    autoStatusUpdate = enabled;
    if (enabled) {
        statusUpdateTime = millis();
    }
}

void DisplayManager::setupMenu(const char* items[], size_t itemCount) {
    if (!items || itemCount == 0 || itemCount > 4) {
        return;
    }
    
    totalMenuItems = itemCount;
    currentMenuItem = 0;
    
    for (size_t i = 0; i < itemCount; i++) {
        if (items[i]) {
            safeCopy(menuItems[i], sizeof(menuItems[i]), items[i]);
        }
    }
}

int DisplayManager::showMenu() {
    if (!initialized || totalMenuItems == 0) {
        return -1;
    }
    
    currentState = STATE_MENU;
    int selectedItem = -1;
    
    while (currentState == STATE_MENU) {
        // Display current menu item
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Menu:"));
        
        lcd.setCursor(0, 1);
        lcd.print(F("> "));
        lcd.print(menuItems[currentMenuItem]);
        
        // Wait for button press
        ButtonType button = waitForButton(10000); // 10 second timeout
        
        switch (button) {
            case BUTTON_UP:
                if (currentMenuItem > 0) {
                    currentMenuItem--;
                }
                break;
                
            case BUTTON_DOWN:
                if (currentMenuItem < totalMenuItems - 1) {
                    currentMenuItem++;
                }
                break;
                
            case BUTTON_SELECT:
                selectedItem = currentMenuItem;
                currentState = STATE_IDLE;
                break;
                
            case BUTTON_LEFT:
                // Cancel menu
                currentState = STATE_IDLE;
                selectedItem = -1;
                break;
                
            case BUTTON_NONE:
                // Timeout
                currentState = STATE_IDLE;
                selectedItem = -1;
                break;
                
            default:
                break;
        }
    }
    
    clearDisplay();
    return selectedItem;
}

DisplayManager::ButtonType DisplayManager::getCurrentButton() const {
    return currentButton;
}

DisplayManager::ButtonType DisplayManager::waitForButton(uint32_t timeoutMs) {
    uint32_t startTime = millis();
    
    // Wait for button release if one is currently pressed
    while (currentButton != BUTTON_NONE && (millis() - startTime) < timeoutMs) {
        updateButtonState();
        delay(10);
    }
    
    // Wait for new button press
    while (currentButton == BUTTON_NONE && (millis() - startTime) < timeoutMs) {
        updateButtonState();
        delay(10);
    }
    
    ButtonType pressedButton = currentButton;
    
    // Wait for button release
    while (currentButton != BUTTON_NONE && (millis() - startTime) < timeoutMs) {
        updateButtonState();
        delay(10);
    }
    
    return pressedButton;
}

bool DisplayManager::isButtonHeld(ButtonType button) const {
    return currentButton == button && buttonHeld;
}

uint32_t DisplayManager::getButtonPressDuration() const {
    if (currentButton != BUTTON_NONE) {
        return millis() - buttonPressTime;
    }
    return 0;
}

void DisplayManager::setBacklight(bool enabled) {
    backlightEnabled = enabled;
    // Note: OSEPP LCD shield doesn't have backlight control pin
    // This would need hardware modification
}

bool DisplayManager::isBacklightEnabled() const {
    return backlightEnabled;
}

void DisplayManager::displayProgressBar(uint8_t percentage, uint8_t line, const char* label) {
    if (!initialized || line > 1) {
        return;
    }
    
    lcd.setCursor(0, line);
    
    if (label) {
        // Display label first (truncated to fit)
        char labelBuffer[9]; // 8 chars + null
        safeCopy(labelBuffer, sizeof(labelBuffer), label, 8);
        lcd.print(labelBuffer);
        lcd.setCursor(8, line);
    }
    
    // Calculate progress bar segments
    uint8_t fullSegments = (percentage * 8) / 100;
    uint8_t partialSegment = ((percentage * 8) % 100) / 20; // 0-4 (5 levels)
    
    // Display progress bar (8 characters)
    for (uint8_t i = 0; i < 8; i++) {
        if (i < fullSegments) {
            lcd.write((uint8_t)5); // Full character
        } else if (i == fullSegments && partialSegment > 0) {
            lcd.write((uint8_t)partialSegment); // Partial character
        } else {
            lcd.write((uint8_t)0); // Empty character
        }
    }
}

void DisplayManager::setupProgressBarChars() {
    for (uint8_t i = 0; i < 8; i++) {
        lcd.createChar(i, progressBarChars[i]);
    }
}

void DisplayManager::displayValue(const char* label, uint32_t value, const char* unit) {
    if (!initialized || !label) {
        return;
    }
    
    char valueStr[17];
    clearBuffer(valueStr, sizeof(valueStr));
    
    // Format: "Label: 1234 unit"
    safeCopy(valueStr, sizeof(valueStr), label);
    appendString(valueStr, sizeof(valueStr), ": ");
    
    char numStr[10];
    intToString(value, numStr, sizeof(numStr));
    appendString(valueStr, sizeof(valueStr), numStr);
    
    if (unit) {
        appendString(valueStr, sizeof(valueStr), " ");
        appendString(valueStr, sizeof(valueStr), unit);
    }
    
    displayMessage(valueStr);
}

void DisplayManager::displayTime(uint8_t hours, uint8_t minutes, uint8_t line) {
    if (!initialized || line > 1) {
        return;
    }
    
    char timeStr[9]; // "HH:MM" + spaces
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);
    
    lcd.setCursor(0, line);
    lcd.print(timeStr);
}

void DisplayManager::displayError(const char* errorMsg, int errorCode) {
    if (!initialized || !errorMsg) {
        return;
    }
    
    char line1[17];
    char line2[17];
    
    safeCopy(line1, sizeof(line1), "ERROR");
    
    if (errorCode != 0) {
        snprintf(line2, sizeof(line2), "%s (%d)", errorMsg, errorCode);
    } else {
        safeCopy(line2, sizeof(line2), errorMsg);
    }
    
    displayMessage(line1, line2, 5000); // 5 second timeout
    
    // Flash error pattern
    for (int i = 0; i < 3; i++) {
        lcd.setCursor(15, 0);
        lcd.write((uint8_t)6); // Error pattern character
        delay(200);
        lcd.setCursor(15, 0);
        lcd.print(" ");
        delay(200);
    }
}

const __FlashStringHelper* DisplayManager::getButtonName(ButtonType button) const {
    switch (button) {
        case BUTTON_RIGHT: return F("RIGHT");
        case BUTTON_UP: return F("UP");
        case BUTTON_DOWN: return F("DOWN");
        case BUTTON_LEFT: return F("LEFT");
        case BUTTON_SELECT: return F("SELECT");
        case BUTTON_NONE:
        default: return F("NONE");
    }
}

bool DisplayManager::testButtons() {
    if (!initialized) {
        return false;
    }
    
    displayMessagePGM(F("Button Test"), F("Press any button"), 0);
    
    for (int i = 0; i < 5; i++) { // Test each button
        ButtonType expectedButton = (ButtonType)(i + 1);
        
        char testMsg[17];
        snprintf(testMsg, sizeof(testMsg), "Press %s", 
                (char*)getButtonName(expectedButton));
        displayMessage("Button Test", testMsg, 0);
        
        ButtonType pressedButton = waitForButton(5000);
        
        if (pressedButton != expectedButton) {
            displayError("Button failed", i + 1);
            return false;
        }
        
        delay(500);
    }
    
    displayMessagePGM(F("Button Test"), F("PASSED"), 2000);
    return true;
}

DisplayManager::DisplayState DisplayManager::getCurrentState() const {
    return currentState;
}

void DisplayManager::forceUpdate() {
    displayUpdateTime = 0; // Force immediate update
    updateDisplay();
}