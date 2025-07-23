#include <unity.h>
#include <stdint.h>

// Mock Arduino functions
#define F(x) x

// Mock status codes
#define STATUS_OK 0
#define STATUS_ERROR 1
#define STATUS_NOT_INITIALIZED 2
#define STATUS_BUSY 3

// Mock IComponent interface for testing
class MockComponent {
public:
    bool initialized;
    int status;
    bool debugEnabled;
    size_t memoryUsage;
    const char* componentName;
    
    MockComponent(const char* name, size_t memory = 100) 
        : initialized(false), status(STATUS_NOT_INITIALIZED), 
          debugEnabled(false), memoryUsage(memory), componentName(name) {}
    
    int initialize() {
        if (!initialized) {
            initialized = true;
            status = STATUS_OK;
        }
        return status;
    }
    
    int update() {
        if (!initialized) return STATUS_NOT_INITIALIZED;
        return STATUS_OK;
    }
    
    int getStatus() const { return status; }
    
    const char* getName() const { return componentName; }
    
    bool validate() const { return initialized && status == STATUS_OK; }
    
    int reset() {
        initialized = false;
        status = STATUS_NOT_INITIALIZED;
        return STATUS_OK;
    }
    
    size_t getMemoryUsage() const { return memoryUsage; }
    
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    
    bool isDebugEnabled() const { return debugEnabled; }
    
    // Simulate failure conditions
    void simulateFailure() { status = STATUS_ERROR; }
    void simulateBusy() { status = STATUS_BUSY; }
};

// Mock ServiceLocator implementation for testing
class ServiceLocatorTest {
private:
    static MockComponent* components[7];
    static bool componentsInitialized;
    static size_t componentCount;
    
public:
    static void registerComponents(MockComponent* comp1, MockComponent* comp2 = nullptr,
                                 MockComponent* comp3 = nullptr, MockComponent* comp4 = nullptr,
                                 MockComponent* comp5 = nullptr, MockComponent* comp6 = nullptr,
                                 MockComponent* comp7 = nullptr) {
        componentCount = 0;
        components[componentCount++] = comp1;
        if (comp2) components[componentCount++] = comp2;
        if (comp3) components[componentCount++] = comp3;
        if (comp4) components[componentCount++] = comp4;
        if (comp5) components[componentCount++] = comp5;
        if (comp6) components[componentCount++] = comp6;
        if (comp7) components[componentCount++] = comp7;
        
        componentsInitialized = true;
    }
    
    static int initializeAll() {
        if (!componentsInitialized) return STATUS_NOT_INITIALIZED;
        
        for (size_t i = 0; i < componentCount; i++) {
            if (components[i]) {
                int result = components[i]->initialize();
                if (result != STATUS_OK) {
                    return result;
                }
            }
        }
        return STATUS_OK;
    }
    
    static int updateAll() {
        if (!componentsInitialized) return STATUS_NOT_INITIALIZED;
        
        for (size_t i = 0; i < componentCount; i++) {
            if (components[i]) {
                int result = components[i]->update();
                if (result != STATUS_OK) {
                    return result;
                }
            }
        }
        return STATUS_OK;
    }
    
    static bool validateAll() {
        if (!componentsInitialized) return false;
        
        for (size_t i = 0; i < componentCount; i++) {
            if (components[i] && !components[i]->validate()) {
                return false;
            }
        }
        return true;
    }
    
    static int resetAll() {
        if (!componentsInitialized) return STATUS_NOT_INITIALIZED;
        
        for (size_t i = 0; i < componentCount; i++) {
            if (components[i]) {
                int result = components[i]->reset();
                if (result != STATUS_OK) {
                    return result;
                }
            }
        }
        return STATUS_OK;
    }
    
    static size_t getTotalMemoryUsage() {
        size_t total = 0;
        for (size_t i = 0; i < componentCount; i++) {
            if (components[i]) {
                total += components[i]->getMemoryUsage();
            }
        }
        return total;
    }
    
    static MockComponent** getAllComponents(size_t& count) {
        count = componentCount;
        return components;
    }
    
    static bool allComponentsRegistered() {
        return componentsInitialized && componentCount > 0;
    }
    
    static MockComponent* getComponentByName(const char* name) {
        for (size_t i = 0; i < componentCount; i++) {
            if (components[i] && strcmp(components[i]->getName(), name) == 0) {
                return components[i];
            }
        }
        return nullptr;
    }
    
    static void setAllDebugEnabled(bool enabled) {
        for (size_t i = 0; i < componentCount; i++) {
            if (components[i]) {
                components[i]->setDebugEnabled(enabled);
            }
        }
    }
    
    static void reset() {
        for (size_t i = 0; i < 7; i++) {
            components[i] = nullptr;
        }
        componentsInitialized = false;
        componentCount = 0;
    }
};

// Static member definitions
MockComponent* ServiceLocatorTest::components[7] = {nullptr};
bool ServiceLocatorTest::componentsInitialized = false;
size_t ServiceLocatorTest::componentCount = 0;

// Test setup/teardown
void setUp(void) {
    ServiceLocatorTest::reset();
}

void tearDown(void) {
    ServiceLocatorTest::reset();
}

// ============================================================================
// Component Registration Tests
// ============================================================================

void test_servicelocator_register_single_component() {
    MockComponent comp1("TestComponent1");
    
    ServiceLocatorTest::registerComponents(&comp1);
    
    TEST_ASSERT_TRUE(ServiceLocatorTest::allComponentsRegistered());
    
    size_t count;
    MockComponent** components = ServiceLocatorTest::getAllComponents(count);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(&comp1, components[0]);
}

void test_servicelocator_register_multiple_components() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    MockComponent comp3("Component3");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2, &comp3);
    
    size_t count;
    MockComponent** components = ServiceLocatorTest::getAllComponents(count);
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(&comp1, components[0]);
    TEST_ASSERT_EQUAL(&comp2, components[1]);
    TEST_ASSERT_EQUAL(&comp3, components[2]);
}

void test_servicelocator_register_max_components() {
    MockComponent comp1("Comp1"), comp2("Comp2"), comp3("Comp3"), comp4("Comp4");
    MockComponent comp5("Comp5"), comp6("Comp6"), comp7("Comp7");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2, &comp3, &comp4, &comp5, &comp6, &comp7);
    
    size_t count;
    ServiceLocatorTest::getAllComponents(count);
    TEST_ASSERT_EQUAL(7, count);
}

// ============================================================================
// Component Initialization Tests
// ============================================================================

void test_servicelocator_initialize_all_success() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    
    int result = ServiceLocatorTest::initializeAll();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_TRUE(comp1.initialized);
    TEST_ASSERT_TRUE(comp2.initialized);
    TEST_ASSERT_EQUAL(STATUS_OK, comp1.getStatus());
    TEST_ASSERT_EQUAL(STATUS_OK, comp2.getStatus());
}

void test_servicelocator_initialize_all_failure() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    // Simulate failure in comp2
    comp2.simulateFailure();
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    
    int result = ServiceLocatorTest::initializeAll();
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
    TEST_ASSERT_TRUE(comp1.initialized);  // First component should succeed
    // Note: In real implementation, comp2 might not be initialized due to early return
}

void test_servicelocator_initialize_not_registered() {
    int result = ServiceLocatorTest::initializeAll();
    
    TEST_ASSERT_EQUAL(STATUS_NOT_INITIALIZED, result);
}

// ============================================================================
// Component Update Tests
// ============================================================================

void test_servicelocator_update_all_success() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    ServiceLocatorTest::initializeAll();  // Initialize first
    
    int result = ServiceLocatorTest::updateAll();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
}

void test_servicelocator_update_all_failure() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    ServiceLocatorTest::initializeAll();
    
    // Simulate failure in comp1
    comp1.simulateFailure();
    
    int result = ServiceLocatorTest::updateAll();
    
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
}

void test_servicelocator_update_not_initialized() {
    int result = ServiceLocatorTest::updateAll();
    
    TEST_ASSERT_EQUAL(STATUS_NOT_INITIALIZED, result);
}

// ============================================================================
// Component Validation Tests
// ============================================================================

void test_servicelocator_validate_all_success() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    ServiceLocatorTest::initializeAll();
    
    bool result = ServiceLocatorTest::validateAll();
    
    TEST_ASSERT_TRUE(result);
}

void test_servicelocator_validate_all_failure() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    ServiceLocatorTest::initializeAll();
    
    // Simulate failure in comp2
    comp2.simulateFailure();
    
    bool result = ServiceLocatorTest::validateAll();
    
    TEST_ASSERT_FALSE(result);
}

void test_servicelocator_validate_not_registered() {
    bool result = ServiceLocatorTest::validateAll();
    
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// Component Reset Tests
// ============================================================================

void test_servicelocator_reset_all_success() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    ServiceLocatorTest::initializeAll();
    
    // Verify components are initialized
    TEST_ASSERT_TRUE(comp1.initialized);
    TEST_ASSERT_TRUE(comp2.initialized);
    
    int result = ServiceLocatorTest::resetAll();
    
    TEST_ASSERT_EQUAL(STATUS_OK, result);
    TEST_ASSERT_FALSE(comp1.initialized);
    TEST_ASSERT_FALSE(comp2.initialized);
    TEST_ASSERT_EQUAL(STATUS_NOT_INITIALIZED, comp1.getStatus());
    TEST_ASSERT_EQUAL(STATUS_NOT_INITIALIZED, comp2.getStatus());
}

// ============================================================================
// Memory Usage Tests
// ============================================================================

void test_servicelocator_memory_usage() {
    MockComponent comp1("Component1", 100);
    MockComponent comp2("Component2", 200);
    MockComponent comp3("Component3", 150);
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2, &comp3);
    
    size_t totalMemory = ServiceLocatorTest::getTotalMemoryUsage();
    
    TEST_ASSERT_EQUAL(450, totalMemory);  // 100 + 200 + 150
}

void test_servicelocator_memory_usage_empty() {
    size_t totalMemory = ServiceLocatorTest::getTotalMemoryUsage();
    
    TEST_ASSERT_EQUAL(0, totalMemory);
}

// ============================================================================
// Component Lookup Tests
// ============================================================================

void test_servicelocator_get_component_by_name() {
    MockComponent comp1("ParallelPortManager");
    MockComponent comp2("DisplayManager");
    MockComponent comp3("FileSystemManager");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2, &comp3);
    
    MockComponent* found = ServiceLocatorTest::getComponentByName("DisplayManager");
    
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(&comp2, found);
    TEST_ASSERT_EQUAL_STRING("DisplayManager", found->getName());
}

void test_servicelocator_get_component_not_found() {
    MockComponent comp1("Component1");
    
    ServiceLocatorTest::registerComponents(&comp1);
    
    MockComponent* found = ServiceLocatorTest::getComponentByName("NonExistentComponent");
    
    TEST_ASSERT_NULL(found);
}

// ============================================================================
// Debug Control Tests
// ============================================================================

void test_servicelocator_set_all_debug_enabled() {
    MockComponent comp1("Component1");
    MockComponent comp2("Component2");
    
    ServiceLocatorTest::registerComponents(&comp1, &comp2);
    
    // Initially debug should be disabled
    TEST_ASSERT_FALSE(comp1.isDebugEnabled());
    TEST_ASSERT_FALSE(comp2.isDebugEnabled());
    
    ServiceLocatorTest::setAllDebugEnabled(true);
    
    TEST_ASSERT_TRUE(comp1.isDebugEnabled());
    TEST_ASSERT_TRUE(comp2.isDebugEnabled());
    
    ServiceLocatorTest::setAllDebugEnabled(false);
    
    TEST_ASSERT_FALSE(comp1.isDebugEnabled());
    TEST_ASSERT_FALSE(comp2.isDebugEnabled());
}

// ============================================================================
// Edge Cases and Error Conditions
// ============================================================================

void test_servicelocator_null_component_handling() {
    MockComponent comp1("Component1");
    
    // Register with nulls mixed in
    ServiceLocatorTest::registerComponents(&comp1, nullptr, nullptr);
    
    size_t count;
    MockComponent** components = ServiceLocatorTest::getAllComponents(count);
    TEST_ASSERT_EQUAL(1, count);  // Should only count non-null components
}

void test_servicelocator_component_lifecycle() {
    MockComponent comp1("Component1");
    
    ServiceLocatorTest::registerComponents(&comp1);
    
    // Test complete lifecycle
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::initializeAll());
    TEST_ASSERT_TRUE(ServiceLocatorTest::validateAll());
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::updateAll());
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::resetAll());
    
    // After reset, component should not be initialized
    TEST_ASSERT_FALSE(comp1.initialized);
}

void test_servicelocator_multiple_initializations() {
    MockComponent comp1("Component1");
    
    ServiceLocatorTest::registerComponents(&comp1);
    
    // Initialize multiple times - should be idempotent
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::initializeAll());
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::initializeAll());
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::initializeAll());
    
    TEST_ASSERT_TRUE(comp1.initialized);
    TEST_ASSERT_EQUAL(STATUS_OK, comp1.getStatus());
}

// ============================================================================
// Integration Tests
// ============================================================================

void test_servicelocator_complete_workflow() {
    MockComponent ppm("ParallelPortManager", 512);
    MockComponent fsm("FileSystemManager", 256);
    MockComponent dm("DisplayManager", 128);
    
    // Register components
    ServiceLocatorTest::registerComponents(&ppm, &fsm, &dm);
    TEST_ASSERT_TRUE(ServiceLocatorTest::allComponentsRegistered());
    
    // Initialize all
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::initializeAll());
    
    // Validate all
    TEST_ASSERT_TRUE(ServiceLocatorTest::validateAll());
    
    // Check memory usage
    TEST_ASSERT_EQUAL(896, ServiceLocatorTest::getTotalMemoryUsage());
    
    // Update cycle
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::updateAll());
    
    // Find specific component
    MockComponent* found = ServiceLocatorTest::getComponentByName("DisplayManager");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(&dm, found);
    
    // Enable debug for all
    ServiceLocatorTest::setAllDebugEnabled(true);
    TEST_ASSERT_TRUE(dm.isDebugEnabled());
    
    // Reset all
    TEST_ASSERT_EQUAL(STATUS_OK, ServiceLocatorTest::resetAll());
    TEST_ASSERT_FALSE(ServiceLocatorTest::validateAll());
}

// ============================================================================
// Test runner
// ============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Registration tests
    RUN_TEST(test_servicelocator_register_single_component);
    RUN_TEST(test_servicelocator_register_multiple_components);
    RUN_TEST(test_servicelocator_register_max_components);
    
    // Initialization tests
    RUN_TEST(test_servicelocator_initialize_all_success);
    RUN_TEST(test_servicelocator_initialize_all_failure);
    RUN_TEST(test_servicelocator_initialize_not_registered);
    
    // Update tests
    RUN_TEST(test_servicelocator_update_all_success);
    RUN_TEST(test_servicelocator_update_all_failure);
    RUN_TEST(test_servicelocator_update_not_initialized);
    
    // Validation tests
    RUN_TEST(test_servicelocator_validate_all_success);
    RUN_TEST(test_servicelocator_validate_all_failure);
    RUN_TEST(test_servicelocator_validate_not_registered);
    
    // Reset tests
    RUN_TEST(test_servicelocator_reset_all_success);
    
    // Memory usage tests
    RUN_TEST(test_servicelocator_memory_usage);
    RUN_TEST(test_servicelocator_memory_usage_empty);
    
    // Component lookup tests
    RUN_TEST(test_servicelocator_get_component_by_name);
    RUN_TEST(test_servicelocator_get_component_not_found);
    
    // Debug control tests
    RUN_TEST(test_servicelocator_set_all_debug_enabled);
    
    // Edge cases
    RUN_TEST(test_servicelocator_null_component_handling);
    RUN_TEST(test_servicelocator_component_lifecycle);
    RUN_TEST(test_servicelocator_multiple_initializations);
    
    // Integration tests
    RUN_TEST(test_servicelocator_complete_workflow);
    
    return UNITY_END();
}