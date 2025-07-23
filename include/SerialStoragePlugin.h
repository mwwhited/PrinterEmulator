#ifndef SERIALSTORAGEPLUGIN_H
#define SERIALSTORAGEPLUGIN_H

#include <Arduino.h>
#include "IStoragePlugin.h"
#include "HardwareConfig.h"

/**
 * Serial Storage Plugin
 * Implements real-time hex streaming protocol for data export
 * Uses BEGIN/END delimiters with CRLF line formatting (64 bytes per line)
 */
class SerialStoragePlugin : public IStoragePlugin {
private:
    bool initialized;
    bool debugEnabled;
    
    // Transfer state
    bool transferInProgress;
    char currentFilename[MAX_FILENAME_LENGTH];
    uint32_t totalBytesTransferred;
    uint32_t totalFilesTransferred;
    
    // Hex formatting buffers - reduced size
    char hexLineBuffer[64];     // Reduced from 128 to save memory
    char hexByteBuffer[4];      // Buffer for single hex byte conversion
    
    // Protocol constants
    static constexpr size_t HEX_BYTES_PER_LINE = 32; // 32 bytes per line (64 hex characters)
    static constexpr char PROTOCOL_BEGIN[] = "BEGIN:";
    static constexpr char PROTOCOL_END[] = "END:";
    static constexpr char PROTOCOL_SIZE[] = "SIZE:";
    static constexpr char PROTOCOL_CRLF[] = "\r\n";
    
    /**
     * Convert byte to hex string
     * @param value Byte value to convert
     * @param dest Buffer to store hex string (must be at least 3 bytes)
     */
    void byteToHex(uint8_t value, char* dest);
    
    /**
     * Convert hex string to byte
     * @param hexStr Hex string (2 characters)
     * @param value Output byte value
     * @return true if conversion successful
     */
    bool hexToByte(const char* hexStr, uint8_t& value);
    
    /**
     * Send protocol header
     * @param filename File name
     * @param fileSize File size in bytes
     */
    void sendProtocolHeader(const char* filename, uint32_t fileSize);
    
    /**
     * Send protocol footer
     * @param filename File name
     */
    void sendProtocolFooter(const char* filename);
    
    /**
     * Send hex data line
     * @param data Data bytes to send
     * @param size Number of bytes (max HEX_BYTES_PER_LINE)
     * @param address Starting address for this line
     */
    void sendHexLine(const uint8_t* data, size_t size, uint32_t address);
    
    /**
     * Wait for serial output to complete
     */
    void flushSerial();
    
    /**
     * Check if serial port is ready for output
     * @return true if serial is ready
     */
    bool isSerialReady() const;
    
public:
    /**
     * Constructor
     */
    SerialStoragePlugin();
    
    /**
     * Destructor
     */
    ~SerialStoragePlugin() override = default;
    
    // IStoragePlugin interface implementation
    int initialize() override;
    bool isReady() const override;
    StorageType getType() const override;
    const __FlashStringHelper* getName() const override;
    uint32_t getAvailableSpace() const override;
    uint32_t getTotalSpace() const override;
    size_t writeFile(const char* filename, const uint8_t* data, size_t size) override;
    size_t readFile(const char* filename, uint8_t* data, size_t maxSize) override;
    bool deleteFile(const char* filename) override;
    bool fileExists(const char* filename) const override;
    uint32_t getFileSize(const char* filename) const override;
    size_t listFiles(char filenames[][MAX_FILENAME_LENGTH], size_t maxFiles) const override;
    bool format() override;
    bool getStatus(char* statusBuffer, size_t bufferSize) const override;
    bool validate() const override;
    size_t getMemoryUsage() const override;
    
    /**
     * Stream file as hex data to serial port
     * @param filename File name to include in protocol
     * @param data Data to stream
     * @param size Size of data
     * @return Number of bytes streamed
     */
    size_t streamFile(const char* filename, const uint8_t* data, size_t size);
    
    /**
     * Receive hex data from serial port and convert to binary
     * @param data Buffer to store received data
     * @param maxSize Maximum bytes to receive
     * @param timeoutMs Timeout in milliseconds
     * @return Number of bytes received
     */
    size_t receiveFile(uint8_t* data, size_t maxSize, uint32_t timeoutMs = 5000);
    
    /**
     * Check if transfer is in progress
     * @return true if transfer is active
     */
    bool isTransferInProgress() const;
    
    /**
     * Abort current transfer
     */
    void abortTransfer();
    
    /**
     * Get transfer statistics
     * @param filesTransferred Number of files transferred
     * @param bytesTransferred Total bytes transferred
     */
    void getTransferStats(uint32_t& filesTransferred, uint32_t& bytesTransferred) const;
    
    /**
     * Reset transfer statistics
     */
    void resetTransferStats();
    
    /**
     * Enable/disable debug output
     * @param enabled true to enable debug output
     */
    void setDebugEnabled(bool enabled);
    
    /**
     * Send progress update during transfer
     * @param filename Current filename
     * @param bytesTransferred Bytes transferred so far
     * @param totalBytes Total bytes to transfer
     */
    void sendProgressUpdate(const char* filename, uint32_t bytesTransferred, uint32_t totalBytes);
    
    /**
     * Test serial protocol with sample data
     * @return true if test successful
     */
    bool testProtocol();
    
    /**
     * Set hex bytes per line (for different terminal capabilities)
     * @param bytesPerLine Bytes per hex line (max 64)
     */
    void setHexBytesPerLine(size_t bytesPerLine);
};

#endif // SERIALSTORAGEPLUGIN_H