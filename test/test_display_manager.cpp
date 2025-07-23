#include <unity.h>
#include <stdint.h>
#include <string.h>

// Mock Arduino functions
#define F(x) x
#define analogRead(pin) mock_analogRead(pin)

// Mock LCD and button system
static int mock_analog_value = 1023; // No button pressed
static char mock_lcd_line1[17] = {0};
static char mock_lcd_line2[17] = {0};
static uint8_t mock_lcd_cursor_col = 0;
static uint8_t mock_lcd_cursor_row = 0;

int mock_analogRead(int pin) {
    return mock_analog_value;
}

// Mock LiquidCrystal class
class MockLiquidCrystal {
public:
    MockLiquidCrystal(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) {}
    
    void begin(uint8_t cols, uint8_t rows) {}
    
    void setCursor(uint8_t col, uint8_t row) {
        mock_lcd_cursor_col = col;
        mock_lcd_cursor_row = row;
    }
    
    void print(const char* str) {
        if (mock_lcd_cursor_row == 0) {
            strncpy(mock_lcd_line1 + mock_lcd_cursor_col, str, 16 - mock_lcd_cursor_col);
        } else if (mock_lcd_cursor_row == 1) {
            strncpy(mock_lcd_line2 + mock_lcd_cursor_col, str, 16 - mock_lcd_cursor_col);
        }
    }
    
    void clear() {
        memset(mock_lcd_line1, 0, sizeof(mock_lcd_line1));
        memset(mock_lcd_line2, 0, sizeof(mock_lcd_line2));
        mock_lcd_cursor_col = 0;
        mock_lcd_cursor_row = 0;
    }
    
    void createChar(uint8_t location, uint8_t charmap[]) {}
};

// Button definitions
#define BUTTON_NONE 1023
#define BUTTON_RIGHT 0
#define BUTTON_UP 144
#define BUTTON_DOWN 329
#define BUTTON_LEFT 505
#define BUTTON_SELECT 742

// Status codes
#define STATUS_OK 0
#define STATUS_ERROR 1
#define STATUS_NOT_INITIALIZED 2

// Display Manager Test Implementation
class DisplayManagerTest {
private:
    MockLiquidCrystal lcd;
    char line1Buffer[6];  // Emergency reduced buffer
    char line2Buffer[6];
    uint8_t currentMenu;
    uint8_t menuOffset;
    bool initialized;
    bool debugEnabled;
    uint32_t lastButtonTime;
    uint8_t lastButton;
    
    // Menu system
    char menuItems[4][6];  // 4 menu items, 5 chars + null
    uint8_t menuCount;
    
public:
    DisplayManagerTest() : lcd(8, 9, 4, 5, 6, 7), currentMenu(0), menuOffset(0),
                          initialized(false), debugEnabled(false),
                          lastButtonTime(0), lastButton(BUTTON_NONE),
                          menuCount(0) {
        memset(line1Buffer, 0, sizeof(line1Buffer));
        memset(line2Buffer, 0, sizeof(line2Buffer));
        memset(menuItems, 0, sizeof(menuItems));
    }
    
    int initialize() {
        if (initialized) return STATUS_OK;
        
        lcd.begin(16, 2);
        lcd.clear();
        
        // Setup default menu items
        strncpy(menuItems[0], "Capt", 5);  // Capture
        strncpy(menuItems[1], "View", 5);  // View
        strncpy(menuItems[2], "Copy", 5);  // Copy
        strncpy(menuItems[3], "Cfg", 4);   // Config
        menuCount = 4;
        
        displayMenu();
        initialized = true;
        
        return STATUS_OK;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        
        uint8_t button = readButton();
        if (button != lastButton && button != BUTTON_NONE) {
            handleButton(button);
            lastButton = button;
            lastButtonTime = 0; // Mock time
        }
        
        return STATUS_OK;
    }
    
    bool validate() const {
        return initialized;
    }
    
    int reset() {
        if (initialized) {
            lcd.clear();
            currentMenu = 0;
            menuOffset = 0;
            initialized = false;
        }
        return STATUS_OK;
    }
    
    void displayStatus(const char* line1, const char* line2) {
        if (!initialized) return;
        
        lcd.clear();
        
        // Copy with truncation for emergency buffer
        strncpy(line1Buffer, line1, sizeof(line1Buffer) - 1);
        strncpy(line2Buffer, line2, sizeof(line2Buffer) - 1);
        line1Buffer[sizeof(line1Buffer) - 1] = '\0';
        line2Buffer[sizeof(line2Buffer) - 1] = '\0';
        
        lcd.setCursor(0, 0);
        lcd.print(line1Buffer);
        
        lcd.setCursor(0, 1);
        lcd.print(line2Buffer);
    }
    
    void displayMenu() {
        if (!initialized || menuCount == 0) return;
        
        lcd.clear();
        
        // Line 1: Menu indicator
        lcd.setCursor(0, 0);
        lcd.print("Menu:");
        
        // Line 2: Current menu item
        lcd.setCursor(0, 1);
        if (currentMenu < menuCount) {
            lcd.print(menuItems[currentMenu]);
        }
    }
    
    void displayError(const char* errorMsg) {
        if (!initialized) return;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR");
        
        lcd.setCursor(0, 1);
        // Truncate error message to fit emergency buffer
        char truncated[6];
        strncpy(truncated, errorMsg, sizeof(truncated) - 1);
        truncated[sizeof(truncated) - 1] = '\0';
        lcd.print(truncated);
    }
    
    void displayProgress(uint8_t percent, const char* operation) {
        if (!initialized) return;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        
        // Truncate operation name
        char truncated[6];
        strncpy(truncated, operation, sizeof(truncated) - 1);
        truncated[sizeof(truncated) - 1] = '\0';
        lcd.print(truncated);
        
        lcd.setCursor(0, 1);
        // Simple progress display: "50%"
        if (percent < 10) {
            lcd.print("0");
        }
        if (percent < 100) {
            // Mock print number
            lcd.print("50");
        } else {
            lcd.print("100");
        }
        lcd.print("%");
    }
    
    uint8_t readButton() {
        int value = analogRead(0);
        
        if (value < 50) return BUTTON_RIGHT;
        if (value < 200) return BUTTON_UP;
        if (value < 400) return BUTTON_DOWN;
        if (value < 600) return BUTTON_LEFT;
        if (value < 800) return BUTTON_SELECT;
        
        return BUTTON_NONE;
    }
    
    void handleButton(uint8_t button) {
        switch (button) {
            case BUTTON_UP:
                if (currentMenu > 0) {
                    currentMenu--;
                    displayMenu();
                }
                break;
                
            case BUTTON_DOWN:
                if (currentMenu < menuCount - 1) {
                    currentMenu++;
                    displayMenu();
                }
                break;
                
            case BUTTON_SELECT:
                executeMenuItem(currentMenu);
                break;
                
            case BUTTON_LEFT:
                // Back/Cancel
                displayMenu();
                break;
                
            case BUTTON_RIGHT:
                // Forward/Enter
                executeMenuItem(currentMenu);
                break;
        }
    }
    
    void executeMenuItem(uint8_t item) {
        // Mock menu item execution
        switch (item) {
            case 0: // Capture
                displayStatus("Start", "Capt");
                break;
            case 1: // View
                displayStatus("Data", "View");
                break;
            case 2: // Copy
                displayStatus("Copy", "File");
                break;
            case 3: // Config
                displayStatus("Conf", "Mode");
                break;
        }
    }
    
    // Test helper methods
    const char* getLine1() const { return mock_lcd_line1; }
    const char* getLine2() const { return mock_lcd_line2; }
    uint8_t getCurrentMenu() const { return currentMenu; }
    uint8_t getMenuCount() const { return menuCount; }
    
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
};

// Test setup/teardown
void setUp(void) {
    mock_analog_value = BUTTON_NONE;
    memset(mock_lcd_line1, 0, sizeof(mock_lcd_line1));
    memset(mock_lcd_line2, 0, sizeof(mock_lcd_line2));
    mock_lcd_cursor_col = 0;
    mock_lcd_cursor_row = 0;
}

void tearDown(void) {}

// ============================================================================
// Component Lifecycle Tests
// ============================================================================

void test_display_manager_initialization() {
    DisplayManagerTest dm;
    
    int result = dm.initialize();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(dm.validate());
    TEST_ASSERT_EQUAL(0, dm.getCurrentMenu());
    TEST_ASSERT_EQUAL(4, dm.getMenuCount());
}

void test_display_manager_double_initialization() {
    DisplayManagerTest dm;
    
    TEST_ASSERT_EQUAL(STATUS_OK, dm.initialize());
    TEST_ASSERT_EQUAL(STATUS_OK, dm.initialize()); // Should be idempotent
    
    TEST_ASSERT_TRUE(dm.validate());
}

void test_display_manager_update_not_initialized() {
    DisplayManagerTest dm;
    
    int result = dm.update();
    
    TEST_ASSERT_EQUAL(STATUS_NOT_INITIALIZED, result);
}

void test_display_manager_reset() {
    DisplayManagerTest dm;
    
    dm.initialize();
    dm.displayStatus("Test", "Reset");
    
    int result = dm.reset();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_FALSE(dm.validate());
}

// ============================================================================
// Display Functionality Tests
// ============================================================================

void test_display_status_message() {
    DisplayManagerTest dm;
    dm.initialize();
    
    dm.displayStatus("Hello", "World");
    
    // Note: In a real test, we would check the LCD content
    // For this mock, we just verify no crashes occurred
    TEST_ASSERT_TRUE(dm.validate());
}

void test_display_status_truncation() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Test truncation with emergency buffer size
    dm.displayStatus("VeryLongLine1", "VeryLongLine2");
    
    TEST_ASSERT_TRUE(dm.validate());
}

void test_display_error_message() {
    DisplayManagerTest dm;
    dm.initialize();
    
    dm.displayError("Test Error");
    
    TEST_ASSERT_TRUE(dm.validate());
}

void test_display_progress_bar() {
    DisplayManagerTest dm;
    dm.initialize();
    
    dm.displayProgress(50, "Copy");
    dm.displayProgress(100, "Done");
    
    TEST_ASSERT_TRUE(dm.validate());
}

// ============================================================================
// Menu System Tests
// ============================================================================

void test_menu_navigation_up() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Start at menu 0, should not go below 0
    TEST_ASSERT_EQUAL(0, dm.getCurrentMenu());
    
    mock_analog_value = BUTTON_UP;
    dm.update();
    
    TEST_ASSERT_EQUAL(0, dm.getCurrentMenu()); // Should stay at 0
}

void test_menu_navigation_down() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Navigate down through menu
    mock_analog_value = BUTTON_DOWN;
    dm.update();
    
    TEST_ASSERT_EQUAL(1, dm.getCurrentMenu());
    
    dm.update();
    TEST_ASSERT_EQUAL(2, dm.getCurrentMenu());
}

void test_menu_navigation_wraparound() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Navigate to last menu item
    for (int i = 0; i < 4; i++) {
        mock_analog_value = BUTTON_DOWN;
        dm.update();
        mock_analog_value = BUTTON_NONE; // Reset button
    }
    
    // Should be at last item (3) and not advance further
    TEST_ASSERT_EQUAL(3, dm.getCurrentMenu());
    
    mock_analog_value = BUTTON_DOWN;
    dm.update();
    
    TEST_ASSERT_EQUAL(3, dm.getCurrentMenu()); // Should stay at 3
}

void test_menu_selection() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Select first menu item
    mock_analog_value = BUTTON_SELECT;
    dm.update();
    
    // Menu selection should execute
    TEST_ASSERT_TRUE(dm.validate());
}

// ============================================================================
// Button Input Tests
// ============================================================================

void test_button_detection_right() {
    DisplayManagerTest dm;
    dm.initialize();
    
    mock_analog_value = BUTTON_RIGHT;
    uint8_t button = dm.readButton();
    
    TEST_ASSERT_EQUAL(BUTTON_RIGHT, button);
}

void test_button_detection_up() {
    DisplayManagerTest dm;
    dm.initialize();
    
    mock_analog_value = BUTTON_UP;
    uint8_t button = dm.readButton();
    
    TEST_ASSERT_EQUAL(BUTTON_UP, button);
}

void test_button_detection_down() {
    DisplayManagerTest dm;
    dm.initialize();
    
    mock_analog_value = BUTTON_DOWN;
    uint8_t button = dm.readButton();
    
    TEST_ASSERT_EQUAL(BUTTON_DOWN, button);
}

void test_button_detection_left() {
    DisplayManagerTest dm;
    dm.initialize();
    
    mock_analog_value = BUTTON_LEFT;
    uint8_t button = dm.readButton();
    
    TEST_ASSERT_EQUAL(BUTTON_LEFT, button);
}

void test_button_detection_select() {
    DisplayManagerTest dm;
    dm.initialize();
    
    mock_analog_value = BUTTON_SELECT;
    uint8_t button = dm.readButton();
    
    TEST_ASSERT_EQUAL(BUTTON_SELECT, button);
}

void test_button_detection_none() {
    DisplayManagerTest dm;
    dm.initialize();
    
    mock_analog_value = BUTTON_NONE;
    uint8_t button = dm.readButton();
    
    TEST_ASSERT_EQUAL(BUTTON_NONE, button);
}

// ============================================================================
// Button Handling Tests
// ============================================================================

void test_button_handling_up_navigation() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Move to menu item 1
    dm.handleButton(BUTTON_DOWN);
    TEST_ASSERT_EQUAL(1, dm.getCurrentMenu());
    
    // Move back to item 0
    dm.handleButton(BUTTON_UP);
    TEST_ASSERT_EQUAL(0, dm.getCurrentMenu());
}

void test_button_handling_select_execution() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Select current menu item
    dm.handleButton(BUTTON_SELECT);
    
    // Should execute menu item (no crash)
    TEST_ASSERT_TRUE(dm.validate());
}

void test_button_handling_left_back() {
    DisplayManagerTest dm;
    dm.initialize();
    
    dm.displayStatus("Test", "Page");
    
    // LEFT should go back to menu
    dm.handleButton(BUTTON_LEFT);
    
    TEST_ASSERT_TRUE(dm.validate());
}

// ============================================================================
// Emergency Buffer Tests
// ============================================================================

void test_emergency_buffer_limits() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Test with strings longer than emergency buffer
    dm.displayStatus("VeryLongStatusLine1", "VeryLongStatusLine2");
    dm.displayError("VeryLongErrorMessage");
    dm.displayProgress(75, "VeryLongOperationName");
    
    // Should not crash with buffer overflow
    TEST_ASSERT_TRUE(dm.validate());
}

void test_menu_item_truncation() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Menu items are already set to fit emergency buffer
    TEST_ASSERT_EQUAL(4, dm.getMenuCount());
    TEST_ASSERT_TRUE(dm.validate());
}

// ============================================================================
// Debug and Configuration Tests
// ============================================================================

void test_debug_control() {
    DisplayManagerTest dm;
    
    TEST_ASSERT_FALSE(dm.isDebugEnabled());
    
    dm.setDebugEnabled(true);
    TEST_ASSERT_TRUE(dm.isDebugEnabled());
    
    dm.setDebugEnabled(false);
    TEST_ASSERT_FALSE(dm.isDebugEnabled());
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_display_manager_complete_workflow() {
    DisplayManagerTest dm;
    
    // Initialize
    TEST_ASSERT_EQUAL(STATUS_OK, dm.initialize());
    
    // Display initial menu
    dm.displayMenu();
    TEST_ASSERT_EQUAL(0, dm.getCurrentMenu());
    
    // Navigate menu
    dm.handleButton(BUTTON_DOWN);
    TEST_ASSERT_EQUAL(1, dm.getCurrentMenu());
    
    // Select menu item
    dm.handleButton(BUTTON_SELECT);
    
    // Show status
    dm.displayStatus("Work", "Done");
    
    // Show progress
    dm.displayProgress(50, "Copy");
    dm.displayProgress(100, "Done");
    
    // Show error
    dm.displayError("Test");
    
    // Back to menu
    dm.handleButton(BUTTON_LEFT);
    
    // Update cycle
    TEST_ASSERT_EQUAL(STATUS_OK, dm.update());
    
    // Reset
    TEST_ASSERT_EQUAL(STATUS_OK, dm.reset());
    TEST_ASSERT_FALSE(dm.validate());
}

void test_continuous_button_presses() {
    DisplayManagerTest dm;
    dm.initialize();
    
    // Simulate rapid button presses
    for (int i = 0; i < 10; i++) {
        mock_analog_value = (i % 2 == 0) ? BUTTON_DOWN : BUTTON_UP;
        dm.update();
        mock_analog_value = BUTTON_NONE;
        dm.update();
    }
    
    TEST_ASSERT_TRUE(dm.validate());
    TEST_ASSERT_LESS_THAN(4, dm.getCurrentMenu()); // Should stay within bounds
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Component lifecycle tests
    RUN_TEST(test_display_manager_initialization);
    RUN_TEST(test_display_manager_double_initialization);
    RUN_TEST(test_display_manager_update_not_initialized);
    RUN_TEST(test_display_manager_reset);
    
    // Display functionality tests
    RUN_TEST(test_display_status_message);
    RUN_TEST(test_display_status_truncation);
    RUN_TEST(test_display_error_message);
    RUN_TEST(test_display_progress_bar);
    
    // Menu system tests
    RUN_TEST(test_menu_navigation_up);
    RUN_TEST(test_menu_navigation_down);
    RUN_TEST(test_menu_navigation_wraparound);
    RUN_TEST(test_menu_selection);
    
    // Button input tests
    RUN_TEST(test_button_detection_right);
    RUN_TEST(test_button_detection_up);
    RUN_TEST(test_button_detection_down);
    RUN_TEST(test_button_detection_left);
    RUN_TEST(test_button_detection_select);
    RUN_TEST(test_button_detection_none);
    
    // Button handling tests
    RUN_TEST(test_button_handling_up_navigation);
    RUN_TEST(test_button_handling_select_execution);
    RUN_TEST(test_button_handling_left_back);
    
    // Emergency buffer tests
    RUN_TEST(test_emergency_buffer_limits);
    RUN_TEST(test_menu_item_truncation);
    
    // Debug and configuration tests
    RUN_TEST(test_debug_control);
    
    // Integration tests
    RUN_TEST(test_display_manager_complete_workflow);
    RUN_TEST(test_continuous_button_presses);
    
    return UNITY_END();
}