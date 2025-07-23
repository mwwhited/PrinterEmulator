#ifndef HEARTBEATLEDMANAGER_H
#define HEARTBEATLEDMANAGER_H

#include <Arduino.h>
#include "IComponent.h"
#include "HardwareConfig.h"

/**
 * HeartbeatLEDManager - Visual system status (placeholder)
 * Will implement heartbeat LED with SOS error patterns
 */
class HeartbeatLEDManager : public IComponent {
private:
    bool initialized;
    bool debugEnabled;
    bool sosMode;
    uint32_t lastBlinkTime;
    bool ledState;
    
public:
    HeartbeatLEDManager() : initialized(false), debugEnabled(false), 
                           sosMode(false), lastBlinkTime(0), ledState(false) {}
    ~HeartbeatLEDManager() override = default;
    
    // IComponent interface implementation
    int initialize() override { 
        pinMode(HEARTBEAT_LED_PIN, OUTPUT);
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
        initialized = true; 
        return STATUS_OK; 
    }
    
    int update() override { 
        if (!initialized) return STATUS_NOT_INITIALIZED;
        
        uint32_t currentTime = millis();
        if (currentTime - lastBlinkTime >= HEARTBEAT_INTERVAL) {
            ledState = !ledState;
            digitalWrite(HEARTBEAT_LED_PIN, ledState ? HIGH : LOW);
            lastBlinkTime = currentTime;
        }
        
        return STATUS_OK; 
    }
    
    int getStatus() const override { 
        return initialized ? STATUS_OK : STATUS_NOT_INITIALIZED; 
    }
    
    const __FlashStringHelper* getName() const override {
        return F("HeartbeatLEDManager");
    }
    
    bool validate() const override { 
        return initialized; 
    }
    
    int reset() override { 
        sosMode = false;
        ledState = false;
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
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
    
    void triggerSOSPattern() {
        sosMode = true;
        // Simple SOS indication for now
        for (int i = 0; i < 3; i++) {
            digitalWrite(HEARTBEAT_LED_PIN, HIGH);
            delay(100);
            digitalWrite(HEARTBEAT_LED_PIN, LOW);
            delay(100);
        }
        delay(300);
        for (int i = 0; i < 3; i++) {
            digitalWrite(HEARTBEAT_LED_PIN, HIGH);
            delay(300);
            digitalWrite(HEARTBEAT_LED_PIN, LOW);
            delay(100);
        }
        delay(300);
        for (int i = 0; i < 3; i++) {
            digitalWrite(HEARTBEAT_LED_PIN, HIGH);
            delay(100);
            digitalWrite(HEARTBEAT_LED_PIN, LOW);
            delay(100);
        }
    }
};

#endif // HEARTBEATLEDMANAGER_H