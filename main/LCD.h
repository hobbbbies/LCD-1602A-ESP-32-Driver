#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// Initialize LCD in 4-bit mode
void lcd_init(void);

// Write data to LCD
void lcd_write(void);

#endif // LCD_H