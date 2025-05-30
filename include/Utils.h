#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

//================================
// DEBUG FUNCTIONS
//================================

// Debug setting
extern bool debugMode;

// Debug print functions - only output if debug mode is enabled
void debugPrint(const char* message);
void debugPrintf(const char* format, ...);

#endif // UTILS_H