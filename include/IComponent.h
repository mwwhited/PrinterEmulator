#ifndef ICOMPONENT_H
#define ICOMPONENT_H

#include <Arduino.h>
#include "HardwareConfig.h"

/**
 * Base interface for all system components
 * Provides standard lifecycle and status management
 */
class IComponent {
public:
    virtual ~IComponent() = default;
    
    /**
     * Initialize the component
     * @return STATUS_OK on success, error code on failure
     */
    virtual int initialize() = 0;
    
    /**
     * Update component (called in main loop)
     * @return STATUS_OK on success, error code on failure
     */
    virtual int update() = 0;
    
    /**
     * Get current component status
     * @return Status code (STATUS_OK, STATUS_ERROR, etc.)
     */
    virtual int getStatus() const = 0;
    
    /**
     * Get component name for debugging
     * @return Component name string (stored in PROGMEM)
     */
    virtual const __FlashStringHelper* getName() const = 0;
    
    /**
     * Validate component state
     * @return true if component is in valid state
     */
    virtual bool validate() const = 0;
    
    /**
     * Reset component to initial state
     * @return STATUS_OK on success, error code on failure
     */
    virtual int reset() = 0;
    
    /**
     * Get memory usage in bytes
     * @return Memory usage or 0 if not applicable
     */
    virtual size_t getMemoryUsage() const = 0;
    
    /**
     * Enable/disable debug output for this component
     * @param enabled true to enable debug output
     */
    virtual void setDebugEnabled(bool enabled) = 0;
    
    /**
     * Check if debug output is enabled
     * @return true if debug output is enabled
     */
    virtual bool isDebugEnabled() const = 0;
};

#endif // ICOMPONENT_H