// === KEYBOARD I2C SLAVE: ATmega328P ===
// Handles 4x10 matrix keyboard with unified I2C protocol
// Communicates over I2C as slave, triggered by master polling
// Supports key rollover - sends all key changes in one transmission
// Uses unified protocol: [TYPE][COUNT][DATA...]

#include <Arduino.h>
#include <Wire.h>

// === Configuration ===
#define I2C_ADDRESS 0x10        // Different from encoder (0x11, 0x12, 0x13, 0x14)
#define MATRIX_ROWS 4           // Number of matrix rows
#define MATRIX_COLS 10          // Number of matrix columns
#define DEBOUNCE_MS 20          // Debounce time in milliseconds (adjustable)
#define MAX_CHANGES 8           // Maximum key changes to buffer per scan

// === Protocol Constants ===
#define DATA_TYPE_ENCODER  0x01  // Data type identifier for encoder messages
#define DATA_TYPE_KEYPRESS 0x02  // Data type identifier for keypress messages
#define DATA_TYPE_BUTTON   0x03  // Data type identifier for button messages (future use)

// === Functions ===
void processKeyChanges();
void setupMatrix();
void sendKeyboardData();
void scanMatrix();


// === Pin Definitions ===
const uint8_t rowPins[MATRIX_ROWS] = {2, 3, 4, 5};           // Row pins (outputs, driven LOW during scan)
const uint8_t colPins[MATRIX_COLS] = {A0, A1, A2, A3, 6, 7, 8, 9, 11, 12}; // Column pins (inputs with pullups)

// === Key Numbering Matrix ===
// Custom numbering scheme: Row 0: 401-410, Row 1: 301-310, Row 2: 201-210, Row 3: 101-110
const uint16_t keyNumbers[MATRIX_ROWS][MATRIX_COLS] = {
  {401, 402, 403, 404, 405, 406, 407, 408, 409, 410},  // Row 0 (top row)
  {301, 302, 303, 304, 305, 306, 307, 308, 309, 310},  // Row 1  
  {201, 202, 203, 204, 205, 206, 207, 208, 209, 210},  // Row 2
  {101, 102, 103, 104, 105, 106, 107, 108, 109, 110}   // Row 3 (bottom row)
};

// === State Tracking Structure ===
struct KeyState {
  bool currentState;       // Current debounced state of the key
  bool lastState;         // Previous state used for change detection
  unsigned long lastChangeTime; // Time of last raw state change (for debouncing)
  bool needsReporting;    // Flag indicating this key needs to be reported
};

// === Key Change Buffer Structure ===
struct KeyChange {
  uint16_t keyNumber;     // Which key changed
  uint8_t newState;       // New state (1=pressed, 0=released)
};

// Initialize state tracking for all keys in the matrix
KeyState keyStates[MATRIX_ROWS][MATRIX_COLS];

// === Key Change Buffer ===
KeyChange keyChangeBuffer[MAX_CHANGES];  // Buffer for multiple key changes
uint8_t pendingChanges = 0;              // Number of changes currently buffered

void setup() {
  // Initialize I2C communication as slave device
  Wire.begin(I2C_ADDRESS);       // Set this device's I2C address
  Wire.onRequest(sendKeyboardData); // Register callback for when master requests data
  
  // Initialize serial communication for debugging
  Serial.begin(115200);           // Start serial
  Serial.println("[I2C SLAVE] Keyboard slave starting...");
  Serial.print("I2C Address: 0x"); // Display I2C address for verification
  Serial.println(I2C_ADDRESS, HEX);
  
  // Initialize matrix hardware pins
  setupMatrix();
  
  // Initialize all key states to default values
  for (int row = 0; row < MATRIX_ROWS; row++) {
    for (int col = 0; col < MATRIX_COLS; col++) {
      keyStates[row][col].currentState = false;     // Key starts unpressed
      keyStates[row][col].lastState = false;        // No previous state
      keyStates[row][col].lastChangeTime = 0;       // No previous change time
      keyStates[row][col].needsReporting = false;   // No changes to report
    }
  }
  
  // Initialize change buffer
  pendingChanges = 0;
  
  Serial.println("[INIT] Matrix initialized, ready for scanning...");
}

void setupMatrix() {
  // Configure row pins as outputs (these will be driven LOW one at a time during scanning)
  for (int i = 0; i < MATRIX_ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);     // Set row pin as output
    digitalWrite(rowPins[i], HIGH);  // Default to HIGH (not currently being scanned)
  }
  
  // Configure column pins as inputs with internal pullup resistors
  // When a key is pressed, it connects the row (driven LOW) to the column,
  // pulling the column LOW. When not pressed, pullup keeps column HIGH.
  for (int i = 0; i < MATRIX_COLS; i++) {
    pinMode(colPins[i], INPUT_PULLUP); // Enable internal pullup resistor
  }
}

void loop() {
  scanMatrix();        // Scan the key matrix for changes
  processKeyChanges(); // Process any detected changes and buffer them
  delay(1);           // Small delay to prevent excessive CPU usage
}

void scanMatrix() {
  unsigned long currentTime = millis(); // Get current time for debouncing
  
  // Scan each row of the matrix one at a time
  for (int row = 0; row < MATRIX_ROWS; row++) {
    // Set only the current row LOW, all others HIGH
    // This allows us to detect which column (if any) is connected to this row
    for (int r = 0; r < MATRIX_ROWS; r++) {
      digitalWrite(rowPins[r], (r == row) ? LOW : HIGH);
    }
    
    // Allow signals to settle after changing pin states
    delayMicroseconds(10);
    
    // Read all columns for the current row
    for (int col = 0; col < MATRIX_COLS; col++) {
      // Key is pressed if column reads LOW (connected to the LOW row through the key)
      // Key is not pressed if column reads HIGH (pulled up by internal resistor)
      bool keyPressed = (digitalRead(colPins[col]) == LOW);
      
      // Implement debouncing: only change state if enough time has passed
      // since the last raw state change. This prevents noise/bouncing from
      // causing false key presses.
      if (keyPressed != keyStates[row][col].currentState) {
        // Check if enough time has passed since last change attempt
        if ((currentTime - keyStates[row][col].lastChangeTime) > DEBOUNCE_MS) {
          // Store the previous state for change detection
          keyStates[row][col].lastState = keyStates[row][col].currentState;
          // Update to the new debounced state
          keyStates[row][col].currentState = keyPressed;
          // Record when this change occurred
          keyStates[row][col].lastChangeTime = currentTime;
          // Mark this key as needing to be reported
          keyStates[row][col].needsReporting = true;
        }
        // If not enough time has passed, we ignore this change (could be bounce)
      }
    }
  }
  
  // After scanning all rows, set all row pins back to HIGH
  // This ensures no keys appear pressed when not actively scanning
  for (int r = 0; r < MATRIX_ROWS; r++) {
    digitalWrite(rowPins[r], HIGH);
  }
}

void processKeyChanges() {
  // Clear any previous pending changes (they should have been sent by now)
  pendingChanges = 0;
  
  // Look through all keys to find ones that have changed state
  for (int row = 0; row < MATRIX_ROWS; row++) {
    for (int col = 0; col < MATRIX_COLS; col++) {
      // If this key's state has changed since last check
      if (keyStates[row][col].needsReporting) {
        // Clear the reporting flag so we don't process this change again
        keyStates[row][col].needsReporting = false;
        
        // Add to buffer if there's room
        if (pendingChanges < MAX_CHANGES) {
          keyChangeBuffer[pendingChanges].keyNumber = keyNumbers[row][col];
          keyChangeBuffer[pendingChanges].newState = keyStates[row][col].currentState ? 1 : 0;
          pendingChanges++;
          
          // Output debug information to serial monitor
          Serial.print("[KEY] ");
          Serial.print(keyNumbers[row][col]); // Print key number
          Serial.print(" ");
          Serial.print(keyStates[row][col].currentState ? "PRESSED" : "RELEASED");
          Serial.print(" (Row: ");
          Serial.print(row);
          Serial.print(", Col: ");
          Serial.print(col);
          Serial.println(")");
        } else {
          // Buffer overflow - this shouldn't happen with reasonable polling rates
          Serial.println("[WARNING] Key change buffer overflow!");
        }
      }
    }
  }
  
  // If we have pending changes, log how many we're buffering
  if (pendingChanges > 0) {
    Serial.print("[BUFFER] ");
    Serial.print(pendingChanges);
    Serial.println(" key change(s) ready for I2C transmission");
  }
}

void sendKeyboardData() {
  // This function is called when the I2C master requests data from us
  
  // Check if we have any key changes to send
  if (pendingChanges > 0) {
    Serial.print("[I2C] Sending ");
    Serial.print(pendingChanges);
    Serial.println(" keypress change(s)");
    
    // Send unified protocol header
    Wire.write(DATA_TYPE_KEYPRESS);  // First byte: indicate this is keypress data
    Wire.write(pendingChanges);      // Second byte: number of key changes
    
    // Send each key event (3 bytes per event)
    for (uint8_t i = 0; i < pendingChanges; i++) {
      Wire.write((keyChangeBuffer[i].keyNumber >> 8) & 0xFF);  // High byte of key number
      Wire.write(keyChangeBuffer[i].keyNumber & 0xFF);         // Low byte of key number  
      Wire.write(keyChangeBuffer[i].newState);                 // State byte (1=pressed, 0=released)
      
      Serial.print("  Key ");
      Serial.print(keyChangeBuffer[i].keyNumber);
      Serial.print(" -> ");
      Serial.println(keyChangeBuffer[i].newState ? "PRESSED" : "RELEASED");
    }
    
    // Clear the buffer after sending
    pendingChanges = 0;
    
    Serial.println("[I2C] Keypress transfer complete");
  } else {
    // No new data to send, just send empty message
    Serial.println("[I2C] No new keypress data to send");
    Wire.write(DATA_TYPE_KEYPRESS);  // First byte: indicate this is keypress data
    Wire.write(0);                   // Second byte: 0 (no key changes)
  }
}