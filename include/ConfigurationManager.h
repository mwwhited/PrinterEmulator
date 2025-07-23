#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include <Arduino.h>
#include "IComponent.h"
#include "HardwareConfig.h"

/**
 * ConfigurationManager - Serial command interface (placeholder)
 * Will implement 30+ debug/control commands for system management
 */
class ConfigurationManager : public IComponent {
private:
    bool initialized;
    bool debugEnabled;
    
public:
    ConfigurationManager() : initialized(false), debugEnabled(false) {}
    ~ConfigurationManager() override = default;
    
    // IComponent interface implementation
    int initialize() override { 
        initialized = true; 
        return STATUS_OK; 
    }
    
    int update() override { 
        return STATUS_OK; 
    }
    
    int getStatus() const override { 
        return initialized ? STATUS_OK : STATUS_NOT_INITIALIZED; 
    }
    
    const __FlashStringHelper* getName() const override {
        return F("ConfigurationManager");
    }
    
    bool validate() const override { 
        return initialized; 
    }
    
    int reset() override { 
        initialized = false; 
        return initialize(); 
    }
    
    size_t getMemoryUsage() const override { 
        return sizeof(*this); 
    }
    
    void setDebugEnabled(bool enabled) override { 
        debugEnabled = enabled; 
    }
    
    bool isDebugEnabled() const override { 
        return debugEnabled; 
    }
};

#endif // CONFIGURATIONMANAGER_H