#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include <rom/ets_sys.h>
#include "freertos/queue.h"
#include "driver/gpio.h"

// Initialize LCD in 4-bit mode
void lcd_init(void);
void lcd_write_character(char c);

#endif // LCD_H