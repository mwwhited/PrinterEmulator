#ifndef HARDWARECONFIG_H
#define HARDWARECONFIG_H

#include <Arduino.h>

// Hardware Pin Assignments
// LCD Shield Interface
#define LCD_RESET_PIN           8
#define LCD_ENABLE_PIN          9
#define LCD_DATA4_PIN           4
#define LCD_DATA5_PIN           5
#define LCD_DATA6_PIN           6
#define LCD_DATA7_PIN           7
#define ANALOG_BUTTONS_PIN      A0  // Pin 54

// Storage & Memory Interface
#define SD_CS_PIN               10
#define EEPROM_CS_PIN           3
#define SD_DETECT_PIN           36  // Active LOW
#define SD_WRITE_PROTECT_PIN    34  // Active HIGH

// SPI Bus Configuration
// ICSP pins are used for SPI (automatically handled by SPI library)
#define SPI_CLOCK_SPEED         4000000  // 4MHz for W25Q128 compatibility

// Real-Time Clock Interface (I2C)
// SDA and SCL pins are automatically handled by Wire library

// Status LEDs
#define HEARTBEAT_LED_PIN       13
#define LPT_ACTIVITY_LED_PIN    30
#define WRITE_ACTIVITY_LED_PIN  32

// IEEE-1284 Parallel Port Interface
#define LPT_STROBE_PIN          18  // /Strobe - Interrupt pin
#define LPT_DATA0_PIN           25  // D0
#define LPT_DATA1_PIN           27  // D1
#define LPT_DATA2_PIN           29  // D2
#define LPT_DATA3_PIN           31  // D3
#define LPT_DATA4_PIN           33  // D4
#define LPT_DATA5_PIN           35  // D5
#define LPT_DATA6_PIN           37  // D6
#define LPT_DATA7_PIN           39  // D7
#define LPT_ACKNOWLEDGE_PIN     41  // /Acknowledge - Output
#define LPT_BUSY_PIN            43  // Busy - Output
#define LPT_PAPER_OUT_PIN       45  // Paper Out - Output (forced low)
#define LPT_SELECT_PIN          47  // Select - Output (forced high)
#define LPT_AUTO_FEED_PIN       22  // /Auto Feed - Input
#define LPT_ERROR_PIN           24  // /Error - Output
#define LPT_INITIALIZE_PIN      26  // /Initialize - Input
#define LPT_SELECT_IN_PIN       28  // /Select In - Input

// Interrupt Configuration
#define LPT_STROBE_INTERRUPT    5   // Pin 18 = INT5 on Mega 2560

// System Constants - Optimized for memory usage
#define RING_BUFFER_SIZE        96   // Strict memory constraint
#define COMMAND_BUFFER_SIZE     16   // Minimum for commands
#define EEPROM_BUFFER_SIZE      16   // Minimum for EEPROM
#define TRANSFER_BUFFER_SIZE    32   // Minimum transfer buffer
#define MAX_FILENAME_LENGTH     8    // 8.3 format maximum

// Timing Constants (microseconds)
#define ACK_PULSE_WIDTH         20
#define HARDWARE_DELAY          5
#define RECOVERY_DELAY          2
#define MODERATE_FLOW_DELAY     25
#define CRITICAL_FLOW_DELAY     50

// Memory Limits
#define TOTAL_RAM_SIZE          8192
#define AVAILABLE_RAM_SIZE      6144    // After stack/heap
#define MAX_FILE_SIZE           16777216 // 16MB EEPROM constraint

// EEPROM Configuration
#define EEPROM_SIZE             16777216 // 16MB W25Q128
#define EEPROM_PAGE_SIZE        256
#define EEPROM_SECTOR_SIZE      4096

// Serial Communication
#define SERIAL_BAUD_RATE        115200
#define SERIAL_DATA_BITS        8
#define SERIAL_STOP_BITS        1

// Debug Configuration
#define DEBUG_ENABLED           1
#define HEARTBEAT_INTERVAL      1000    // milliseconds

// Button Values (OSEPP LCD Shield)
#define BUTTON_RIGHT_VALUE      0
#define BUTTON_UP_VALUE         144
#define BUTTON_DOWN_VALUE       329
#define BUTTON_LEFT_VALUE       505
#define BUTTON_SELECT_VALUE     741
#define BUTTON_NONE_VALUE       1023
#define BUTTON_TOLERANCE        30

// Component Status Codes
#define STATUS_OK               0
#define STATUS_ERROR            1
#define STATUS_NOT_INITIALIZED  2
#define STATUS_BUSY             3
#define STATUS_TIMEOUT          4

#endif // HARDWARECONFIG_H