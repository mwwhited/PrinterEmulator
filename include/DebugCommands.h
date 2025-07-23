#ifndef DEBUGCOMMANDS_H
#define DEBUGCOMMANDS_H

namespace DebugCommands {

/**
 * Initialize debug command system
 */
void initialize();

/**
 * Update debug command processing
 * Call this from main loop to process serial commands
 */
void update();

} // namespace DebugCommands

#endif // DEBUGCOMMANDS_H