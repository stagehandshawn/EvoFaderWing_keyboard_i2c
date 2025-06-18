

# EvoFaderWingKeyboard matrix
- Sends key press and release over i2c to faderwing master
Uses atmega328p 3.3v 8mhz version

- This repo supports the full project [EvoFaderWing](https://github.com/stagehandshawn/EvoFaderWing)

```
View from front

  Col 0                                                          Col 9
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

### Column Order (left to right from front):

| Column | Pin  |
|--------|------|
| 0      | 2   |
| 1      | 3   |
| 2      | 4   |
| 3      | 5   |
| 4      | 6    |
| 5      | 7    |
| 6      | 8    |
| 7      | 9    |
| 8      | 11   |
| 9      | 12   |

### Row Order (top to bottom):

| Row | Pin |
|-----|-----|
| 0   | A0   |
| 1   | A1   |
| 2   | A2   |
| 3   | A3   |

## Wiring diagram
- Keyboard Matrix
  - Coming soon...

- Keyboard and touch sensor 
![Keyboard and touch sensor board](https://github.com/stagehandshawn/EvoFaderWing_keyboard_i2c/blob/main/images/evofaderwing_keyboard_touch_wiring.png)