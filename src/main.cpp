// === COMPLETE KEYBOARD I2C SLAVE: ATmega328P ===
// Handles 4x10 matrix keyboard with circular buffer for multiple keypresses
// Single file version - everything included

#include <Arduino.h>
#include <Wire.h>
#include "Utils.h"

// === Configuration ===
#define I2C_ADDRESS 0x10        
#define MATRIX_ROWS 4           
#define MATRIX_COLS 10          
#define DEBOUNCE_MS 20          
#define CHANGE_BUFFER_SIZE 4    // Small buffer for multiple keypresses

// === Protocol Constants ===
#define DATA_TYPE_KEYPRESS 0x02  

// === Pin Definitions ===
const uint8_t rowPins[MATRIX_ROWS] = {A0, A1, A2, A3};           
const uint8_t colPins[MATRIX_COLS] = {2, 3, 4, 5, 6, 7, 8, 9, 11, 12}; 

// === Key Numbering Matrix ===
const uint16_t keyNumbers[MATRIX_ROWS][MATRIX_COLS] = {
  {401, 402, 403, 404, 405, 406, 407, 408, 409, 410},  
  {301, 302, 303, 304, 305, 306, 307, 308, 309, 310},  
  {201, 202, 203, 204, 205, 206, 207, 208, 209, 210},  
  {101, 102, 103, 104, 105, 106, 107, 108, 109, 110}   
};


// === State Tracking Structure ===
struct KeyState {
  bool currentState;       
  bool lastState;         
  unsigned long lastChangeTime; 
};

// === Circular Buffer Structure ===
struct KeyChange {
  uint16_t keyNumber;     
  uint8_t newState;       
  unsigned long timestamp;
};

// === Global Variables ===
KeyState keyStates[MATRIX_ROWS][MATRIX_COLS];
KeyChange changeBuffer[CHANGE_BUFFER_SIZE];
uint8_t bufferHead = 0;      // Where to write next change
uint8_t bufferTail = 0;      // Where to read next change  
uint8_t bufferCount = 0;     // How many changes are buffered

// === Function Declarations ===
void setupMatrix();
void scanMatrix();
void sendKeyboardData();
void addKeyChange(uint16_t keyNumber, uint8_t newState);
uint8_t getBufferedChangeCount();
bool getNextChange(KeyChange* change);
void clearStaleChanges();

// === SETUP FUNCTION ===
void setup() {
  Wire.begin(I2C_ADDRESS);       
  Wire.onRequest(sendKeyboardData); 
  
  Serial.begin(57600);           
  debugPrint("[KEYBOARD] Circular buffer keyboard slave starting...");
  debugPrintf("I2C Address: 0x%02X", I2C_ADDRESS);
  
  setupMatrix();
  
  // Initialize all key states
  for (int row = 0; row < MATRIX_ROWS; row++) {
    for (int col = 0; col < MATRIX_COLS; col++) {
      keyStates[row][col].currentState = false;     
      keyStates[row][col].lastState = false;        
      keyStates[row][col].lastChangeTime = 0;       
    }
  }
  
  // Initialize change buffer
  bufferHead = 0;
  bufferTail = 0;
  bufferCount = 0;
  
  debugPrint("[KEYBOARD] Matrix initialized, ready for scanning...");
}

// === MAIN LOOP ===
void loop() {
  scanMatrix();        
  
  // Clear stale changes if they've been sitting too long
  clearStaleChanges();
  
  delay(2);  // Slower scanning - 2ms instead of 1ms
}

// === MATRIX SETUP ===
void setupMatrix() {
  for (int i = 0; i < MATRIX_ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);     
    digitalWrite(rowPins[i], HIGH);  
  }
  
  for (int i = 0; i < MATRIX_COLS; i++) {
    pinMode(colPins[i], INPUT_PULLUP); 
  }
}

// === MATRIX SCANNING ===
void scanMatrix() {
  unsigned long currentTime = millis(); 
  
  // Scan each row
  for (int row = 0; row < MATRIX_ROWS; row++) {
    // Set only current row LOW
    for (int r = 0; r < MATRIX_ROWS; r++) {
      digitalWrite(rowPins[r], (r == row) ? LOW : HIGH);
    }
    
    delayMicroseconds(10);
    
    // Read all columns for current row
    for (int col = 0; col < MATRIX_COLS; col++) {
      bool keyPressed = (digitalRead(colPins[col]) == LOW);
      
      // Debouncing
      if (keyPressed != keyStates[row][col].currentState) {
        if ((currentTime - keyStates[row][col].lastChangeTime) > DEBOUNCE_MS) {
          keyStates[row][col].lastState = keyStates[row][col].currentState;
          keyStates[row][col].currentState = keyPressed;
          keyStates[row][col].lastChangeTime = currentTime;
          
          // Add to buffer using helper function
          addKeyChange(keyNumbers[row][col], keyPressed ? 1 : 0);
          
          debugPrintf("[KEY] %d %s (buffered: %d)", 
                     keyNumbers[row][col],
                     keyPressed ? "PRESSED" : "RELEASED",
                     bufferCount);
        }
      }
    }
  }
  
  // Set all rows HIGH after scanning
  for (int r = 0; r < MATRIX_ROWS; r++) {
    digitalWrite(rowPins[r], HIGH);
  }
}

// === I2C DATA TRANSMISSION ===
void sendKeyboardData() {
  // Always send keypress type - never encoder data!
  Wire.write(DATA_TYPE_KEYPRESS);  
  
  uint8_t changesAvailable = getBufferedChangeCount();
  
  if (changesAvailable > 0) {
    // Send up to all available changes (the buffer is small)
    Wire.write(changesAvailable);  
    
    debugPrintf("[I2C] Sending %d key changes", changesAvailable);
    
    // Send each buffered change
    for (uint8_t i = 0; i < changesAvailable; i++) {
      KeyChange change;
      if (getNextChange(&change)) {
        Wire.write((change.keyNumber >> 8) & 0xFF);  // High byte
        Wire.write(change.keyNumber & 0xFF);         // Low byte  
        Wire.write(change.newState);                 // State
        
        debugPrintf("  Key %d -> %s", 
                   change.keyNumber,
                   change.newState ? "PRESSED" : "RELEASED");
      }
    }
    
    debugPrintf("[I2C] Sent %d changes, %d remaining in buffer", 
               changesAvailable, bufferCount);
  } else {
    // No changes to send
    Wire.write(0);  // Count: 0 changes
    debugPrint("[I2C] No changes to send");
  }
}

// === CIRCULAR BUFFER HELPER FUNCTIONS ===

void addKeyChange(uint16_t keyNumber, uint8_t newState) {
  // Add change to circular buffer
  changeBuffer[bufferHead].keyNumber = keyNumber;
  changeBuffer[bufferHead].newState = newState;
  changeBuffer[bufferHead].timestamp = millis();
  
  // Move head pointer
  bufferHead = (bufferHead + 1) % CHANGE_BUFFER_SIZE;
  
  // If buffer is full, advance tail (overwrite oldest)
  if (bufferCount >= CHANGE_BUFFER_SIZE) {
    bufferTail = (bufferTail + 1) % CHANGE_BUFFER_SIZE;
    debugPrint("[BUFFER] Buffer full - overwriting oldest change");
  } else {
    bufferCount++;
  }
}

uint8_t getBufferedChangeCount() {
  return bufferCount;
}

bool getNextChange(KeyChange* change) {
  if (bufferCount == 0) {
    return false;  // No changes available
  }
  
  // Copy change from tail position
  *change = changeBuffer[bufferTail];
  
  // Move tail pointer and decrease count
  bufferTail = (bufferTail + 1) % CHANGE_BUFFER_SIZE;
  bufferCount--;
  
  return true;
}

void clearStaleChanges() {
  unsigned long currentTime = millis();
  
  // Check if oldest change is too old (100ms timeout)
  while (bufferCount > 0) {
    if ((currentTime - changeBuffer[bufferTail].timestamp) > 100) {
      debugPrint("[TIMEOUT] Clearing stale change from buffer");
      bufferTail = (bufferTail + 1) % CHANGE_BUFFER_SIZE;
      bufferCount--;
    } else {
      break;  // Oldest change is still fresh
    }
  }
}