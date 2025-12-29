# LCD-1602A Driver
This repository contains a custom LCD-1602A-compatible LCD driver written for ESP32 using ESP-IDF.

The goal of this project was not to make the shortest LCD driver possible, but to build one that is:

* correct with respect to the datasheet

* non-blocking after initialization

* easy to reason about and extend

Most example LCD drivers rely on long blocking delays and implicit timing. This driver instead models the LCD as a stateful device and drives it using a state machine + non blocking software timers.

# How It Works
## Init
Driver init uses a bootstrap initialization sequence derived from the official LCD-1602A datasheet.
* Sets up 4 bit interface protocol for communication
* Resets display
* Configures entry mode for writing

Initialization is intentionally blocking.

At power-up, the LCD’s internal state is unknown, and the datasheet explicitly requires fixed delays and repeated commands to force it into a known mode.

Once the LCD enters 4-bit mode, the driver switches to non-blocking operation.

## Communication
Communication is done in a non blocking fashion via a state machine + RTOS queue system
* Instructions are stored in a queue that contains all metadata for the instruction; instruction byte, instruction execution time, and a ```data_reg``` flag that controls the RS pin on the LCD (for instruction vs data register)
* ```execute_next_lcd_queue``` kicks the queue to start a chain of state machine timer callbacks; ensuring non blocking, safe data transfer
* ```lcd_send_nibble``` sends a nibble of the instruction byte and pulses the e pin on the LCD-1602A board, completing the data trasnfer. Because of the 4 bit interface, this function must be called twice, for the upper and lower half of the byte. 

### State Machine 
Instead of blocking with delays, the driver progresses through explicit states:

* send high nibble

* send low nibble

* wait for execution time

Each step is scheduled with esp_timer.
This makes timing explicit and avoids overlapping instructions.

The driver uses the following states internally:

```LCD_IDLE``` – ready to accept a new byte

```LCD_SENDING_HIGH``` – high nibble is being sent

```LCD_SENDING_LOW``` – low nibble is being sent

```LCD_EXECUTING``` – LCD is busy internally

The state always reflects the actual LCD hardware state, not just what the code is doing.

## API
A ```void lcd_write_character(char c)``` function is exposed in the driver API to allow the main source code to write characters to the LCD.
```send_to_lcd``` task in main reads from the serial stdout, and calls this api function for instantaneous keyboard --> LCD connection

## Installation
***Requirements***: LCD 1602A display, ESP32 board, basic wiring (see LCD1602A wiring on google for good reference), and ESP-IDF setup on your system
```bash 
git clone <repo url>
idf.py build flash monitor 
```

