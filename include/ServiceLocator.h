#ifndef SERVICELOCATOR_H
#define SERVICELOCATOR_H

#include <Arduino.h>
#include "IComponent.h"

// Forward declarations
class ParallelPortManager;
class FileSystemManager;
class DisplayManager;
class ConfigurationManager;
class TimeManager;
class SystemManager;
class HeartbeatLEDManager;

/**
 * ServiceLocator pattern implementation for component management
 * Provides centralized access to all system components
 * Uses static allocation for zero-allocation architecture
 */
class ServiceLocator {
private:
    // Component pointers
    static ParallelPortManager* parallelPortManager;
    static FileSystemManager* fileSystemManager;
    static DisplayManager* displayManager;
    static ConfigurationManager* configurationManager;
    static TimeManager* timeManager;
    static SystemManager* systemManager;
    static HeartbeatLEDManager* heartbeatLEDManager;
    
    // Cached component array for iteration
    static IComponent* components[7];
    static bool componentsInitialized;
    
    // Private constructor - static class
    ServiceLocator() = delete;
    
public:
    /**
     * Register all components with the service locator
     * Must be called during system initialization
     */
    static void registerComponents(
        ParallelPortManager* ppm,
        FileSystemManager* fsm,
        DisplayManager* dm,
        ConfigurationManager* cm,
        TimeManager* tm,
        SystemManager* sm,
        HeartbeatLEDManager* hlm
    );
    
    /**
     * Get ParallelPortManager instance
     * @return Pointer to ParallelPortManager or nullptr if not registered
     */
    static ParallelPortManager* getParallelPortManager();
    
    /**
     * Get FileSystemManager instance
     * @return Pointer to FileSystemManager or nullptr if not registered
     */
    static FileSystemManager* getFileSystemManager();
    
    /**
     * Get DisplayManager instance
     * @return Pointer to DisplayManager or nullptr if not registered
     */
    static DisplayManager* getDisplayManager();
    
    /**
     * Get ConfigurationManager instance
     * @return Pointer to ConfigurationManager or nullptr if not registered
     */
    static ConfigurationManager* getConfigurationManager();
    
    /**
     * Get TimeManager instance
     * @return Pointer to TimeManager or nullptr if not registered
     */
    static TimeManager* getTimeManager();
    
    /**
     * Get SystemManager instance
     * @return Pointer to SystemManager or nullptr if not registered
     */
    static SystemManager* getSystemManager();
    
    /**
     * Get HeartbeatLEDManager instance
     * @return Pointer to HeartbeatLEDManager or nullptr if not registered
     */
    static HeartbeatLEDManager* getHeartbeatLEDManager();
    
    /**
     * Get array of all registered components
     * @param count Output parameter for number of components
     * @return Array of component pointers
     */
    static IComponent** getAllComponents(size_t& count);
    
    /**
     * Initialize all registered components
     * @return STATUS_OK if all components initialized successfully
     */
    static int initializeAll();
    
    /**
     * Update all registered components
     * @return STATUS_OK if all components updated successfully
     */
    static int updateAll();
    
    /**
     * Validate all registered components
     * @return true if all components are in valid state
     */
    static bool validateAll();
    
    /**
     * Reset all registered components
     * @return STATUS_OK if all components reset successfully
     */
    static int resetAll();
    
    /**
     * Get total memory usage of all components
     * @return Total memory usage in bytes
     */
    static size_t getTotalMemoryUsage();
    
    /**
     * Check if all components are registered
     * @return true if all expected components are registered
     */
    static bool allComponentsRegistered();
    
    /**
     * Get component by name (for debugging)
     * @param name Component name to find
     * @return Pointer to component or nullptr if not found
     */
    static IComponent* getComponentByName(const char* name);
    
    /**
     * Enable/disable debug output for all components
     * @param enabled true to enable debug output
     */
    static void setAllDebugEnabled(bool enabled);
};

#endif // SERVICELOCATOR_H