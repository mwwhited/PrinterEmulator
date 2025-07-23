#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "IComponent.h"
#include "HardwareConfig.h"

/**
 * DisplayManager - 16x2 LCD with button navigation
 * Manages OSEPP LCD Keypad Shield with analog button interface
 * Provides message display, menu navigation, and real-time status updates
 */
class DisplayManager : public IComponent {
public:
    enum ButtonType {
        BUTTON_NONE = 0,
        BUTTON_RIGHT,
        BUTTON_UP,
        BUTTON_DOWN,
        BUTTON_LEFT,
        BUTTON_SELECT
    };
    
    enum DisplayState {
        STATE_IDLE,
        STATE_MENU,
        STATE_MESSAGE,
        STATE_STATUS,
        STATE_SCROLLING
    };
    
private:
    LiquidCrystal lcd;
    
    // State management
    bool initialized;
    bool debugEnabled;
    DisplayState currentState;
    
    // Button handling
    ButtonType lastButton;
    ButtonType currentButton;
    uint32_t buttonPressTime;
    uint32_t buttonReleaseTime;
    bool buttonHeld;
    
    // Display content - optimized sizes
    char line1Buffer[17];       // 16 chars + null terminator
    char line2Buffer[17];
    char messageBuffer[17];     // Reduced from 33 to save memory
    uint32_t displayUpdateTime;
    uint32_t messageTimeout;
    bool messageTimedOut;
    
    // Menu system - reduced size
    size_t currentMenuItem;
    size_t totalMenuItems;
    char menuItems[4][17];      // Reduced from 8 to 4 menu items
    
    // Scrolling text
    size_t scrollPosition;
    uint32_t scrollUpdateTime;
    bool scrollEnabled;
    
    // Status display
    uint32_t statusUpdateTime;
    bool autoStatusUpdate;
    
    // Backlight control
    bool backlightEnabled;
    
    /**
     * Read analog button value and determine pressed button
     * @return Button type pressed
     */
    ButtonType readButton();
    
    /**
     * Update button state with debouncing
     */
    void updateButtonState();
    
    /**
     * Clear display and buffers
     */
    void clearDisplay();
    
    /**
     * Update display content based on current state
     */
    void updateDisplay();
    
    /**
     * Handle button press for current state
     * @param button Button that was pressed
     */
    void handleButtonPress(ButtonType button);
    
    /**
     * Update scrolling text display
     */
    void updateScrolling();
    
    /**
     * Format status line for display
     * @param statusText Status text to format
     * @param buffer Buffer to store formatted text
     * @param bufferSize Size of buffer
     */
    void formatStatusLine(const char* statusText, char* buffer, size_t bufferSize);
    
    /**
     * Center text in display line
     * @param text Text to center
     * @param buffer Buffer to store centered text
     * @param bufferSize Size of buffer
     */
    void centerText(const char* text, char* buffer, size_t bufferSize);
    
    /**
     * Truncate text to fit display width
     * @param text Text to truncate
     * @param buffer Buffer to store truncated text
     * @param bufferSize Size of buffer
     */
    void truncateText(const char* text, char* buffer, size_t bufferSize);
    
public:
    /**
     * Constructor
     */
    DisplayManager();
    
    /**
     * Destructor
     */
    ~DisplayManager() override = default;
    
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
     * Display message on LCD
     * @param line1 First line text (16 chars max)
     * @param line2 Second line text (16 chars max, optional)
     * @param timeoutMs Message timeout in milliseconds (0 for no timeout)
     */
    void displayMessage(const char* line1, const char* line2 = nullptr, uint32_t timeoutMs = 3000);
    
    /**
     * Display message from PROGMEM
     * @param line1 First line PROGMEM text
     * @param line2 Second line PROGMEM text (optional)
     * @param timeoutMs Message timeout in milliseconds
     */
    void displayMessagePGM(const __FlashStringHelper* line1, 
                          const __FlashStringHelper* line2 = nullptr, 
                          uint32_t timeoutMs = 3000);
    
    /**
     * Display scrolling message (for text longer than 16 chars)
     * @param message Long message to scroll
     * @param line Line number (0 or 1)
     * @param scrollSpeed Scroll speed in milliseconds per character
     */
    void displayScrollingMessage(const char* message, uint8_t line = 0, uint32_t scrollSpeed = 300);
    
    /**
     * Display system status
     * @param statusLine1 First status line
     * @param statusLine2 Second status line
     */
    void displayStatus(const char* statusLine1, const char* statusLine2 = nullptr);
    
    /**
     * Clear display and return to idle state
     */
    void clearAndIdle();
    
    /**
     * Enable/disable automatic status updates
     * @param enabled true to enable auto updates
     * @param updateInterval Update interval in milliseconds
     */
    void setAutoStatusUpdate(bool enabled, uint32_t updateInterval = 2000);
    
    /**
     * Set up menu system
     * @param items Array of menu item strings
     * @param itemCount Number of menu items
     */
    void setupMenu(const char* items[], size_t itemCount);
    
    /**
     * Show menu and handle navigation
     * @return Selected menu item index or -1 if cancelled
     */
    int showMenu();
    
    /**
     * Get current button state
     * @return Currently pressed button
     */
    ButtonType getCurrentButton() const;
    
    /**
     * Wait for button press with timeout
     * @param timeoutMs Timeout in milliseconds
     * @return Button pressed or BUTTON_NONE if timeout
     */
    ButtonType waitForButton(uint32_t timeoutMs = 5000);
    
    /**
     * Check if button is currently held down
     * @param button Button to check
     * @return true if button is held
     */
    bool isButtonHeld(ButtonType button) const;
    
    /**
     * Get button press duration
     * @return Duration in milliseconds
     */
    uint32_t getButtonPressDuration() const;
    
    /**
     * Enable/disable backlight
     * @param enabled true to enable backlight
     */
    void setBacklight(bool enabled);
    
    /**
     * Check if backlight is enabled
     * @return true if backlight is on
     */
    bool isBacklightEnabled() const;
    
    /**
     * Display progress bar
     * @param percentage Progress percentage (0-100)
     * @param line Line number (0 or 1)
     * @param label Optional label text
     */
    void displayProgressBar(uint8_t percentage, uint8_t line = 1, const char* label = nullptr);
    
    /**
     * Create custom characters for progress bar
     */
    void setupProgressBarChars();
    
    /**
     * Display numeric value with label
     * @param label Label text
     * @param value Numeric value
     * @param unit Unit string (optional)
     */
    void displayValue(const char* label, uint32_t value, const char* unit = nullptr);
    
    /**
     * Display time in HH:MM format
     * @param hours Hours (0-23)
     * @param minutes Minutes (0-59)
     * @param line Line number (0 or 1)
     */
    void displayTime(uint8_t hours, uint8_t minutes, uint8_t line = 0);
    
    /**
     * Display error message with visual indication
     * @param errorMsg Error message
     * @param errorCode Optional error code
     */
    void displayError(const char* errorMsg, int errorCode = 0);
    
    /**
     * Get button name as string
     * @param button Button type
     * @return Button name string (PROGMEM)
     */
    const __FlashStringHelper* getButtonName(ButtonType button) const;
    
    /**
     * Test all buttons and display status
     * @return true if all buttons respond correctly
     */
    bool testButtons();
    
    /**
     * Get current display state
     * @return Current display state
     */
    DisplayState getCurrentState() const;
    
    /**
     * Force display update (refresh content)
     */
    void forceUpdate();
};

#endif // DISPLAYMANAGER_H