#ifndef HARDWARECOMPONENTTESTS_H
#define HARDWARECOMPONENTTESTS_H

namespace HardwareComponentTests {

/**
 * Test Serial Communication Port
 * @return true if serial communication is working properly
 */
bool testSerialCommunication();

/**
 * Test LCD Display Hardware
 * @return true if LCD is functioning correctly
 */
bool testLCDDisplay();

/**
 * Test Button Interface
 * @return true if buttons are working properly
 */
bool testButtonInterface();

/**
 * Test LED Indicators
 * @return true if all LEDs are functional
 */
bool testLEDIndicators();

/**
 * Test Parallel Port Signals
 * @return true if parallel port pins are working
 */
bool testParallelPortSignals();

/**
 * Test Memory Systems
 * @return true if memory systems are healthy
 */
bool testMemorySystems();

/**
 * Test Storage Systems
 * @return true if storage systems are functional
 */
bool testStorageSystems();

/**
 * Run comprehensive hardware test suite
 * @return true if all tests pass
 */
bool runComprehensiveTests();

} // namespace HardwareComponentTests

#endif // HARDWARECOMPONENTTESTS_H