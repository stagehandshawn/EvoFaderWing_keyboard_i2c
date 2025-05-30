#include "Utils.h"

//================================
// DEBUG SETTINGS
//================================
#ifdef DEBUG
  bool debugMode = true;  // Enables or disables verbose serial output
#endif


//================================
// DEBUG FUNCTIONS
//================================


void debugPrint(const char* message) {
  if (debugMode) {
    Serial.println(message);
  }
}

void debugPrintf(const char* format, ...) {
  if (debugMode) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Check if the format string already ends with a newline
    size_t len = strlen(format);
    if (len > 0 && format[len-1] == '\n') {
      Serial.print(buffer); // Already has newline
    } else {
      Serial.println(buffer); // Add newline
    }
  }
}