# MegaDeviceBridge - Application Design Specification

## Table of Contents
1. [System Overview](#system-overview)
2. [Hardware Architecture](#hardware-architecture)
3. [Software Architecture](#software-architecture)
4. [Use Cases & Operations](#use-cases--operations)
5. [Technical Specifications](#technical-specifications)
6. [Component Details](#component-details)
7. [Interface Protocols](#interface-protocols)
8. [Storage Architecture](#storage-architecture)
9. [Serial Commands](#serial-commands)
10. [Development Guidelines](#development-guidelines)

## System Overview

### Purpose
The **MegaDeviceBridge** is an embedded data acquisition system designed to capture parallel port data from a **Tektronix TDS2024 oscilloscope** and store it in modern digital formats. The system bridges legacy parallel port instrumentation with contemporary storage technologies.

### Key Features
- **Real-time Data Capture**: IEEE-1284 compliant parallel port interface (≤2μs ISR)
- **Multi-Storage Architecture**: SD Card, EEPROM (16MB W25Q128), and Serial Transfer
- **Zero-Allocation Memory Management**: Static buffers with bounds checking
- **Universal Format Support**: All 16 TDS2024 file formats (BMP, PCX, TIFF, RLE, etc.)
- **Enterprise-Grade Architecture**: Component-based design with ServiceLocator pattern
- **Advanced File Transfer**: Inter-storage copying with automatic format conversion
- **Professional Debug Interface**: 30+ serial commands for monitoring and control

### Target Hardware
- **Primary**: Tektronix TDS2024 Digital Storage Oscilloscope
- **Secondary**: Any IEEE-1284 SPP compatible device
- **Data Types**: Binary images, printer files, and raw oscilloscope data

## Hardware Architecture

### Base Platform
- **MCU**: Arduino Mega 2560 (ATmega2560)
  - **Clock**: 16MHz
  - **RAM**: 8KB SRAM (current usage: 66.8% / 5,469 bytes)
  - **Flash**: 256KB (current usage: 40.6% / 103,040 bytes)
  - **Voltage**: 5V logic level
  - **Architecture**: Loop-based cooperative multitasking (no RTOS)

### Shield Stack Configuration
1. **Base Layer**: Arduino Mega 2560
2. **Layer 1**: OSEPP LCD Keypad Shield (revision 1)
3. **Layer 2**: Deek Robot Data Logging Shield v1.0 (rewired for Mega compatibility)

### External Components
- **EEPROM**: Winbond W25Q128FVSG (16MB SPI Flash memory)
- **RTC**: DS1307 Real-Time Clock with battery backup
- **Storage**: SD Card slot (up to 32GB SDHC supported)
- **Display**: 16x2 character LCD with analog button interface
- **Parallel Port**: IEEE-1284 Standard Parallel Port (Centronics/DB25)

## Hardware Pin Assignments

### LCD Shield Interface
| Function | Arduino Pin | Notes |
|----------|-------------|-------|
| LCD Reset | Pin 8 | Display initialization |
| LCD Enable | Pin 9 | Display control |
| LCD Data 4-7 | Pins 4-7 | 4-bit parallel data |
| Analog Buttons | A0 (Pin 54) | OSEPP button array |

### Storage & Memory Interface
| Function | Arduino Pin | Protocol | Notes |
|----------|-------------|----------|-------|
| SD Card CS | Pin 10 | SPI | Chip select |
| EEPROM CS | Pin 3 | SPI | W25Q128 chip select |
| SD Card Detect | Pin 36 | Digital | Active LOW |
| SD Write Protect | Pin 34 | Digital | Active HIGH |
| SPI Clock | ICSP SCLK | SPI | Shared SPI bus |
| SPI MOSI | ICSP MOSI | SPI | Shared SPI bus |
| SPI MISO | ICSP MISO | SPI | Shared SPI bus |

### Real-Time Clock Interface
| Function | Arduino Pin | Protocol | Notes |
|----------|-------------|----------|-------|
| RTC SDA | I2C SDA | I2C | DS1307 data |
| RTC SCL | I2C SCL | I2C | DS1307 clock |

### Status LEDs
| Function | Arduino Pin | Purpose |
|----------|-------------|---------|
| Heartbeat | Pin 13 | System status / SOS error |
| LPT Read Activity | Pin 30 | Parallel port data capture |
| Write Activity | Pin 32 | File write operations |

### IEEE-1284 Parallel Port Interface
| Signal | DB25 Pin | Arduino Pin | Direction | Function |
|--------|----------|-------------|-----------|----------|
| /Strobe | 1 | Pin 18 | Input | Data strobe (interrupt) |
| D0-D7 | 2-9 | Pins 25,27,29,31,33,35,37,39 | Input | 8-bit parallel data |
| /Acknowledge | 10 | Pin 41 | Output | Data acknowledgment |
| Busy | 11 | Pin 43 | Output | Buffer status |
| Paper Out | 12 | Pin 45 | Output | Status (forced low) |
| Select | 13 | Pin 47 | Output | Status (forced high) |
| /Auto Feed | 14 | Pin 22 | Input | Control signal |
| /Error | 15 | Pin 24 | Output | Error status |
| /Initialize | 16 | Pin 26 | Input | Reset signal |
| /Select In | 17 | Pin 28 | Input | Selection signal |
| Ground | 18-25 | GND | Power | Signal ground |

## Software Architecture

### Core Design Principles
1. **Zero-Allocation Memory Management**: Static buffers, no dynamic allocation
2. **Component-Based Architecture**: Modular design with clear interfaces
3. **Enterprise Patterns**: ServiceLocator, Factory, and Plugin architectures
4. **Memory Safety**: Bounds checking on all string operations
5. **Real-Time Constraints**: ≤2μs interrupt service routines
6. **IEEE-1284 Compliance**: Hardware timing specifications

### System Components

#### 1. ParallelPortManager
- **Purpose**: Real-time data capture from TDS2024
- **Features**: 
  - FALLING edge interrupt on /Strobe (Pin 18)
  - Atomic 8-bit parallel data reading
  - 512-byte ring buffer with overflow protection
  - Hardware flow control (Busy/Acknowledge)
  - ≤2μs ISR execution time

#### 2. FileSystemManager
- **Purpose**: Multi-storage file operations
- **Storage Types**:
  - **SD Card**: Primary storage (up to 32GB)
  - **EEPROM**: 16MB W25Q128 minimal filesystem
  - **Serial Transfer**: Real-time hex streaming
- **Features**:
  - Plugin-based architecture
  - Automatic format detection
  - Hot-swap SD card support
  - Write protection detection

#### 3. DisplayManager
- **Purpose**: LCD and user interface
- **Features**:
  - 16x2 character display
  - Button-based navigation
  - Real-time status updates
  - Message timeout handling
  - Debug output throttling

#### 4. ConfigurationManager
- **Purpose**: Serial command interface
- **Features**:
  - 30+ debug/control commands
  - Zero-allocation command parsing
  - Memory-safe parameter extraction
  - System configuration control

#### 5. TimeManager
- **Purpose**: Real-time clock operations
- **Features**:
  - DS1307 RTC integration
  - Automatic filename generation
  - Time-based file organization
  - Battery backup support

#### 6. SystemManager
- **Purpose**: System health monitoring
- **Features**:
  - Memory usage tracking
  - Component status validation
  - Error detection and reporting
  - System self-tests

#### 7. HeartbeatLEDManager
- **Purpose**: Visual system status
- **Features**:
  - Normal operation heartbeat
  - SOS error pattern (...---...)
  - Component failure indication
  - Configurable blink patterns

## Use Cases & Operations

### Primary Use Case: TDS2024 Data Capture

#### Setup Phase
1. **Hardware Connection**: Connect TDS2024 parallel port to Arduino via DB25 cable
2. **Storage Selection**: Insert SD card or use EEPROM storage
3. **System Initialization**: Power on Arduino, verify LCD display shows "Ready"
4. **Time Configuration**: Set RTC using serial command `time set YYYY-MM-DD HH:MM`

#### Data Capture Phase
1. **TDS2024 Configuration**: Configure oscilloscope for desired output format
2. **Trigger Capture**: Press TDS2024 "Print" or "Copy" button
3. **Real-Time Processing**:
   - /Strobe interrupt triggers data capture
   - 8-bit parallel data read atomically
   - Data buffered in 512-byte ring buffer
   - Hardware flow control manages data rate
4. **File Storage**: Data automatically saved with timestamp filename

#### File Management Phase
1. **Storage Operations**: Switch between SD/EEPROM using `storage` command
2. **File Listing**: View stored files using `list` command
3. **File Transfer**: Copy files between storages using `copyto` command
4. **Serial Export**: Stream files as hex data for external processing

### Secondary Use Cases

#### System Monitoring
- **Status Checking**: Use `status` command for component health
- **Memory Monitoring**: Track RAM usage and free space
- **Hardware Validation**: Run `validate` command for component tests
- **Debug Control**: Enable/disable component-specific debug output

#### File Operations
- **Cross-Storage Transfer**: Move files between SD card and EEPROM
- **Format Conversion**: Automatic binary-to-hex conversion for serial transfer
- **Batch Operations**: Process multiple files with pattern matching
- **Archive Management**: Time-based file organization with directory support

#### Development & Debug
- **Component Testing**: Individual component validation and testing
- **Timing Analysis**: Parallel port timing verification
- **Memory Profiling**: Real-time memory usage analysis
- **Hardware Testing**: LED control, button response, sensor readings

## Technical Specifications

### Performance Metrics
- **ISR Execution Time**: ≤2μs (IEEE-1284 compliant)
- **Data Throughput**: Up to 500KB/s (limited by TDS2024)
- **Memory Usage**: 66.8% RAM (5,469/8,192 bytes)
- **Flash Usage**: 40.6% Flash (103,040/253,952 bytes)
- **Power Consumption**: ~200mA @ 5V (typical operation)

### Timing Specifications
- **ACK Pulse Width**: 20μs (optimized for TDS2024)
- **Hardware Delay**: 5μs (data stability)
- **Recovery Delay**: 2μs (between operations)
- **Flow Control**: 25μs moderate, 50μs critical delays

### Memory Architecture
- **Static Allocation**: Zero dynamic memory allocation
- **Buffer Sizes**:
  - Ring Buffer: 512 bytes
  - Command Buffer: 64 bytes
  - EEPROM Buffer: 32 bytes
  - Transfer Buffer: 128-256 bytes
- **Bounds Checking**: All string operations validated
- **Overflow Protection**: Buffer size limits enforced

### Data Integrity
- **Error Detection**: Checksum validation on transfers
- **Flow Control**: Hardware-based data rate management
- **Timeout Handling**: Configurable operation timeouts
- **Recovery Mechanisms**: Automatic error recovery

## Storage Architecture

### Three-Tier Storage Hierarchy

#### Tier 1: SD Card Storage
- **Capacity**: Up to 32GB SDHC
- **Access**: FAT16/FAT32 filesystem
- **Features**:
  - Hot-swap detection
  - Write protection support
  - Directory organization
  - Large file support
- **Use Case**: Primary storage for large datasets

#### Tier 2: EEPROM Minimal Filesystem
- **Capacity**: 16MB (W25Q128FVSG)
- **Access**: Custom minimal filesystem
- **Features**:
  - Flash memory constraints handling
  - Complement-based size encoding
  - Wear leveling support
  - Fast access times
- **Use Case**: Backup storage, system logs

#### Tier 3: Serial Transfer
- **Capacity**: Real-time streaming
- **Access**: Hex-encoded protocol
- **Features**:
  - BEGIN/END delimiters
  - CRLF line formatting (64 bytes)
  - Progress reporting
  - Flow control
- **Use Case**: Real-time monitoring, data export

### File Transfer System
- **Inter-Storage Copying**: `copyto {storage} {filename}`
- **Automatic Format Conversion**: Binary ↔ Hex conversion
- **Memory-Safe Operations**: Direct byte-by-byte processing
- **Error Handling**: Graceful failure recovery

## Interface Protocols

### IEEE-1284 Standard Parallel Port (SPP)
```
Timing Diagram:
  /Strobe  \_____/‾‾‾‾‾‾‾‾‾
  Data     ========[DATA]===
  Busy     /‾‾‾‾‾‾\_________
  /Ack     ‾‾‾‾‾‾‾\___/‾‾‾‾
           |<-5μs->|<-20μs->|
```

#### Protocol Sequence
1. **Data Setup**: TDS2024 places data on D0-D7 lines
2. **Strobe Assert**: /Strobe goes LOW (triggers interrupt)
3. **Data Capture**: Arduino reads data within 2μs
4. **Busy Assert**: Arduino sets Busy HIGH
5. **Acknowledge**: Arduino pulses /Ack LOW for 20μs
6. **Busy Release**: Arduino sets Busy LOW
7. **Ready**: System ready for next byte

### Serial Communication Protocol
- **Baud Rate**: 115,200 bps
- **Format**: 8-N-1 (8 data bits, no parity, 1 stop bit)
- **Flow Control**: Software (XON/XOFF not implemented)
- **Command Format**: ASCII text commands with CRLF termination

### SPI Bus Configuration
- **Clock Speed**: 4MHz (W25Q128 compatible)
- **Mode**: SPI Mode 0 (CPOL=0, CPHA=0)
- **Bit Order**: MSB first
- **Chip Selects**: Active LOW

## Serial Commands Reference

### System Commands
| Command | Parameters | Description |
|---------|------------|-------------|
| `help` | None | Display command menu |
| `info` | None | System information |
| `status` | None | Detailed component status |
| `validate` | None | Hardware self-test |
| `restart` | None | Software reset |

### Storage Commands
| Command | Parameters | Description |
|---------|------------|-------------|
| `storage` | None | Show current storage status |
| `storage` | `sd\|eeprom\|serial\|auto` | Switch storage type |
| `list` | `[storage]` | List files |
| `copyto` | `{storage} {filename}` | Copy file between storages |
| `testwrite` | None | Write test file |

### Time Commands
| Command | Parameters | Description |
|---------|------------|-------------|
| `time` | None | Show current time |
| `time set` | `YYYY-MM-DD HH:MM` | Set RTC time |

### Hardware Commands
| Command | Parameters | Description |
|---------|------------|-------------|
| `parallel` | None | Show parallel port status |
| `testint` | None | Test interrupt for 10 seconds |
| `testlpt` | None | Test LPT protocol signals |
| `buttons` | None | Show button values |
| `led` | `l1\|l2 on\|off` | Manual LED control |
| `led status` | None | Show LED states |

### Debug Commands
| Command | Parameters | Description |
|---------|------------|-------------|
| `debug lcd` | `on\|off\|status` | LCD debug output |
| `debug parallel` | `on\|off\|status` | Parallel port debug |
| `debug eeprom` | `on\|off\|status` | EEPROM debug |
| `heartbeat` | `on\|off\|status` | Serial status messages |

## Component Details

### Memory Management
- **Zero-Allocation Architecture**: No dynamic memory allocation
- **Static Buffers**: Pre-allocated at compile time
- **Bounds Checking**: All string operations validated
- **Custom Utilities**:
  - `safeCopy(dest, destSize, src, maxCopy)`: Safe string copying
  - `startsWith(str, strLen, prefix)`: Prefix checking
  - `equalsIgnoreCase(str1, str1Len, str2)`: Case-insensitive comparison

### Error Handling
- **SOS LED Pattern**: Visual error indication (...---...)
- **Serial Error Messages**: Detailed error reporting
- **Timeout Handling**: Configurable operation timeouts
- **Recovery Mechanisms**: Automatic system recovery
- **Null Pointer Protection**: ServiceLocator validation

### Real-Time Constraints
- **Interrupt Priority**: Parallel port has highest priority
- **ISR Optimization**: Minimal processing in interrupt context
- **Cooperative Multitasking**: Non-preemptive task scheduling
- **Timing Critical Sections**: Hardware delays for TDS2024 compatibility

## Development Guidelines

### Code Standards
- **F() Macro**: ALL string literals must use F("text") for FLASH storage
- **Memory Safety**: Bounds checking on all buffer operations
- **No Dynamic Allocation**: Use static buffers and arrays only
- **Component Interface**: Implement IComponent for all system components
- **ServiceLocator Pattern**: Use cached pointers, avoid runtime lookups

### Testing Requirements
- **Hardware Validation**: Test with real TDS2024 oscilloscope
- **Memory Profiling**: Monitor RAM usage during operations
- **Timing Verification**: Validate ISR execution times
- **Data Integrity**: Verify captured data matches source
- **Stress Testing**: Extended operation under load

### Performance Optimization
- **ISR Minimization**: Keep interrupt handlers under 2μs
- **Cache Service Pointers**: Avoid repeated ServiceLocator calls
- **Buffer Optimization**: Match buffer sizes to hardware constraints
- **Flash Memory Usage**: Store constants in FLASH not RAM
- **Compiler Optimization**: Use constexpr for compile-time constants

### Debugging Techniques
- **Serial Commands**: Use comprehensive debug interface
- **LED Indicators**: Visual feedback for system state
- **Logic Analyzer**: Hardware timing verification
- **Memory Monitoring**: Track usage patterns and leaks
- **Component Testing**: Individual module validation

## System Limits & Constraints

### Hardware Limitations
- **RAM**: 8KB total (6KB after stack/heap)
- **Flash**: 256KB (248KB available)
- **EEPROM**: 4KB internal (not used)
- **SPI Speed**: 4MHz maximum for W25Q128
- **Interrupt Latency**: Must be ≤2μs for IEEE-1284

### Software Constraints
- **No RTOS**: Loop-based cooperative multitasking only
- **No Dynamic Allocation**: Static memory management
- **Limited Floating Point**: Minimal FPU usage
- **Stack Depth**: Avoid deep recursion
- **String Handling**: Custom utilities, no Arduino String class

### Operational Limits
- **File Size**: 16MB maximum (EEPROM constraint)
- **Filename Length**: 32 characters maximum
- **Buffer Overflow**: 512-byte ring buffer limit
- **Transfer Rate**: Limited by TDS2024 output speed
- **Power Budget**: 200mA typical, 300mA maximum

## Future Enhancements

### Planned Features
- **Network Interface**: Ethernet/WiFi connectivity
- **Web Interface**: Browser-based configuration and monitoring
- **Data Compression**: Real-time compression for storage efficiency
- **Multi-Device Support**: Multiple parallel port channels
- **Advanced Triggers**: Conditional data capture

### Scalability Considerations
- **Memory Expansion**: External SRAM for larger buffers
- **Storage Expansion**: Multiple SD cards or USB storage
- **Processing Power**: ARM Cortex-M upgrade path
- **Interface Expansion**: USB, Ethernet, or wireless modules

---

**Document Version**: 1.0  
**Last Updated**: 2025-07-23  
**Status**: Complete - Zero-Allocation Memory Architecture + COPYTO Functionality  
**Author**: Claude Code Assistant  
**Review Status**: Ready for Production Deployment