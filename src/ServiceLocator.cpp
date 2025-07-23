#include "ServiceLocator.h"
#include "MemoryUtils.h"

// Include all manager headers (will be created later)
#include "ParallelPortManager.h"
#include "FileSystemManager.h"
#include "DisplayManager.h"
#include "ConfigurationManager.h"
#include "TimeManager.h"
#include "SystemManager.h"
#include "HeartbeatLEDManager.h"

// Static member definitions
ParallelPortManager* ServiceLocator::parallelPortManager = nullptr;
FileSystemManager* ServiceLocator::fileSystemManager = nullptr;
DisplayManager* ServiceLocator::displayManager = nullptr;
ConfigurationManager* ServiceLocator::configurationManager = nullptr;
TimeManager* ServiceLocator::timeManager = nullptr;
SystemManager* ServiceLocator::systemManager = nullptr;
HeartbeatLEDManager* ServiceLocator::heartbeatLEDManager = nullptr;

IComponent* ServiceLocator::components[7] = {nullptr};
bool ServiceLocator::componentsInitialized = false;

void ServiceLocator::registerComponents(
    ParallelPortManager* ppm,
    FileSystemManager* fsm,
    DisplayManager* dm,
    ConfigurationManager* cm,
    TimeManager* tm,
    SystemManager* sm,
    HeartbeatLEDManager* hlm
) {
    parallelPortManager = ppm;
    fileSystemManager = fsm;
    displayManager = dm;
    configurationManager = cm;
    timeManager = tm;
    systemManager = sm;
    heartbeatLEDManager = hlm;
    
    // Initialize components array for iteration
    components[0] = static_cast<IComponent*>(parallelPortManager);
    components[1] = static_cast<IComponent*>(fileSystemManager);
    components[2] = static_cast<IComponent*>(displayManager);
    components[3] = static_cast<IComponent*>(configurationManager);
    components[4] = static_cast<IComponent*>(timeManager);
    components[5] = static_cast<IComponent*>(systemManager);
    components[6] = static_cast<IComponent*>(heartbeatLEDManager);
    
    componentsInitialized = true;
}

ParallelPortManager* ServiceLocator::getParallelPortManager() {
    return parallelPortManager;
}

FileSystemManager* ServiceLocator::getFileSystemManager() {
    return fileSystemManager;
}

DisplayManager* ServiceLocator::getDisplayManager() {
    return displayManager;
}

ConfigurationManager* ServiceLocator::getConfigurationManager() {
    return configurationManager;
}

TimeManager* ServiceLocator::getTimeManager() {
    return timeManager;
}

SystemManager* ServiceLocator::getSystemManager() {
    return systemManager;
}

HeartbeatLEDManager* ServiceLocator::getHeartbeatLEDManager() {
    return heartbeatLEDManager;
}

IComponent** ServiceLocator::getAllComponents(size_t& count) {
    count = 7;
    return components;
}

int ServiceLocator::initializeAll() {
    if (!componentsInitialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    for (size_t i = 0; i < 7; i++) {
        if (components[i] != nullptr) {
            int result = components[i]->initialize();
            if (result != STATUS_OK) {
                return result;
            }
        }
    }
    
    return STATUS_OK;
}

int ServiceLocator::updateAll() {
    if (!componentsInitialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    for (size_t i = 0; i < 7; i++) {
        if (components[i] != nullptr) {
            int result = components[i]->update();
            if (result != STATUS_OK) {
                return result;
            }
        }
    }
    
    return STATUS_OK;
}

bool ServiceLocator::validateAll() {
    if (!componentsInitialized) {
        return false;
    }
    
    for (size_t i = 0; i < 7; i++) {
        if (components[i] != nullptr) {
            if (!components[i]->validate()) {
                return false;
            }
        }
    }
    
    return true;
}

int ServiceLocator::resetAll() {
    if (!componentsInitialized) {
        return STATUS_NOT_INITIALIZED;
    }
    
    for (size_t i = 0; i < 7; i++) {
        if (components[i] != nullptr) {
            int result = components[i]->reset();
            if (result != STATUS_OK) {
                return result;
            }
        }
    }
    
    return STATUS_OK;
}

size_t ServiceLocator::getTotalMemoryUsage() {
    if (!componentsInitialized) {
        return 0;
    }
    
    size_t totalUsage = 0;
    for (size_t i = 0; i < 7; i++) {
        if (components[i] != nullptr) {
            totalUsage += components[i]->getMemoryUsage();
        }
    }
    
    return totalUsage;
}

bool ServiceLocator::allComponentsRegistered() {
    return parallelPortManager != nullptr &&
           fileSystemManager != nullptr &&
           displayManager != nullptr &&
           configurationManager != nullptr &&
           timeManager != nullptr &&
           systemManager != nullptr &&
           heartbeatLEDManager != nullptr;
}

IComponent* ServiceLocator::getComponentByName(const char* name) {
    if (!componentsInitialized || !name) {
        return nullptr;
    }
    
    size_t nameLen = safeStrlen(name, 32);
    
    for (size_t i = 0; i < 7; i++) {
        if (components[i] != nullptr) {
            // Compare component name (stored in PROGMEM)
            if (equalsIgnoreCasePGM(name, nameLen, components[i]->getName())) {
                return components[i];
            }
        }
    }
    
    return nullptr;
}

void ServiceLocator::setAllDebugEnabled(bool enabled) {
    if (!componentsInitialized) {
        return;
    }
    
    for (size_t i = 0; i < 7; i++) {
        if (components[i] != nullptr) {
            components[i]->setDebugEnabled(enabled);
        }
    }
}