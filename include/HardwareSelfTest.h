#ifndef HARDWARESELFTEST_H
#define HARDWARESELFTEST_H

namespace HardwareSelfTest {

/**
 * Run complete hardware self-test
 * Tests all major hardware components and systems
 * @return true if all tests pass
 */
bool runCompleteSelfTest();

/**
 * Quick system health check
 * Fast validation of critical components
 * @return true if system is healthy
 */
bool quickHealthCheck();

} // namespace HardwareSelfTest

#endif // HARDWARESELFTEST_H