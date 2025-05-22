

# GMA3 Faderwing Keyboard Matrix Firmware

This firmware is designed for an **ATmega328P (3.3V, 8 MHz)** to serve as an I²C slave for a custom keyboard matrix used in a GrandMA3 fader wing controller. It scans a 4×10 key matrix and reports key press/release events to a master device (like a Teensy 4.1) over I²C using a unified protocol.

---

## 🧠 How It Works

- Scans 40 keys arranged in 4 rows and 10 columns.
- Software debounce is applied to each key.
- Only changes (presses or releases) are buffered.
- When the master polls via I²C, the slave replies with all buffered events using this format:

  ```
  [DATA_TYPE_KEYPRESS][COUNT][keyHi_1][keyLo_1][state_1]...[keyHi_N][keyLo_N][state_N]
  ```

---

## 📡 I²C Communication

- **I²C Address**: `0x10`
- **Data Type for Key Messages**: `0x02`
- Each key event is 3 bytes:
  - 2-byte key number (e.g., `401`, `110`)
  - 1-byte state (`1 = pressed`, `0 = released`)

---

## 🔌 Matrix Wiring

- **Row pins** (outputs): `2`, `3`, `4`, `5`
- **Column pins** (inputs with pullups): `A0`, `A1`, `A2`, `A3`, `6`, `7`, `8`, `9`, `11`, `12`

### Key Numbering Layout

```
(Row 0 - top)       (Row 3 - bottom)
+------+------+------+------+------+------+------+------+------+------+
| 401  | 402  | 403  | 404  | 405  | 406  | 407  | 408  | 409  | 410  | ← Row 0
+------+------+------+------+------+------+------+------+------+------+
| 301  | 302  | 303  | 304  | 305  | 306  | 307  | 308  | 309  | 310  | ← Row 1
+------+------+------+------+------+------+------+------+------+------+
| 201  | 202  | 203  | 204  | 205  | 206  | 207  | 208  | 209  | 210  | ← Row 2
+------+------+------+------+------+------+------+------+------+------+
| 101  | 102  | 103  | 104  | 105  | 106  | 107  | 108  | 109  | 110  | ← Row 3
+------+------+------+------+------+------+------+------+------+------+
```

### Column Order (left to right):

| Column | Pin  |
|--------|------|
| 0      | A0   |
| 1      | A1   |
| 2      | A2   |
| 3      | A3   |
| 4      | 6    |
| 5      | 7    |
| 6      | 8    |
| 7      | 9    |
| 8      | 11   |
| 9      | 12   |

### Row Order (top to bottom):

| Row | Pin |
|-----|-----|
| 0   | 2   |
| 1   | 3   |
| 2   | 4   |
| 3   | 5   |

---

## ⚙️ Configuration Summary

| Define          | Purpose                                | Value    |
|----------------|----------------------------------------|----------|
| `I2C_ADDRESS`   | I²C address of this slave              | `0x10`   |
| `MATRIX_ROWS`   | Rows in the matrix                     | `4`      |
| `MATRIX_COLS`   | Columns in the matrix                  | `10`     |
| `DEBOUNCE_MS`   | Milliseconds of debounce delay         | `20`     |
| `MAX_CHANGES`   | Max key changes per I²C transmission   | `8`      |

---

## 🛠 PlatformIO Configuration

Example `platformio.ini`:

```ini
[env:pro8MHzatmega328]
platform = atmelavr
board = pro8MHzatmega328
framework = arduino
lib_deps = Wire
upload_speed = 57600
monitor_speed = 115200
build_flags = -DDEBUG
```

---

## 🧪 Debugging

When `-DDEBUG` is set, serial debug output is printed at **57600 baud**, showing:

- Key number and state (PRESSED/RELEASED)
- Number of keys buffered
- I²C transmission logs

Use a USB-UART adapter for monitoring during development.