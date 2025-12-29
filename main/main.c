#include <stdio.h>
#include "LCD.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

TaskHandle_t serial_handle = NULL;

void read_serial(void* args) {
    while(1) {
        int c = getchar();
        if (c != 0 && c != EOF) {
            // Allocate on heap so it persists across tasks
            char* user_buffer = (char*)pvPortMalloc(2);
            if (user_buffer != NULL) {
                user_buffer[0] = (char)c;
                user_buffer[1] = '\0';
                xTaskNotify(serial_handle, (uint32_t)user_buffer, eSetValueWithOverwrite);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void send_to_lcd(void *parameters) {
    while(1) {
        uint32_t raw;
        if (xTaskNotifyWait(0, 0, &raw, portMAX_DELAY) == pdPASS) {
            char* user_buffer = (char*)raw;
            lcd_write_character(*user_buffer);
            // Free heap memory allocated in collectUserBuffer
            vPortFree(user_buffer);
        }
    }
}

void app_main(void)
{
    lcd_init();
    xTaskCreate(read_serial, "read_serial", 2048, NULL, 1, NULL);
    xTaskCreate(send_to_lcd, "send_to_lcd", 2048, NULL, 1, &serial_handle);
}

