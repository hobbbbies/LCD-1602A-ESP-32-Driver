#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "LCD.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "LCD_TEST";

void setUp(void) {
    // This is run before EACH test
}

void tearDown(void) {
    // This is run after EACH test
}

// Test LCD initialization
void test_lcd_init_succeeds(void) {
    ESP_LOGI(TAG, "Testing LCD initialization");
    
    // Initialize LCD - should complete without errors
    lcd_init();
    
    // Give it time to complete initialization sequence
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // If we get here without crashing, initialization succeeded
    TEST_PASS();
}

// Test writing a single character
void test_lcd_write_single_character(void) {
    ESP_LOGI(TAG, "Testing single character write");
    
    lcd_write_character('A');
    
    // Allow time for state machine to process
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // If we get here without crashing, write succeeded
    TEST_PASS();
}

// Test writing multiple characters
void test_lcd_write_multiple_characters(void) {
    ESP_LOGI(TAG, "Testing multiple character writes");
    
    char test_string[] = "Hello";
    
    for (int i = 0; i < strlen(test_string); i++) {
        lcd_write_character(test_string[i]);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // Allow time for all writes to complete
    vTaskDelay(pdMS_TO_TICKS(200));
    
    TEST_PASS();
}

// Test writing special characters
void test_lcd_write_special_characters(void) {
    ESP_LOGI(TAG, "Testing special character writes");
    
    char special_chars[] = {'0', '9', ' ', '!', '@'};
    
    for (int i = 0; i < sizeof(special_chars); i++) {
        lcd_write_character(special_chars[i]);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    vTaskDelay(pdMS_TO_TICKS(200));
    
    TEST_PASS();
}

// Test rapid writes
void test_lcd_rapid_writes(void) {
    ESP_LOGI(TAG, "Testing rapid character writes");
    
    // Write 10 characters quickly
    for (int i = 0; i < 10; i++) {
        lcd_write_character('X');
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Give queue time to process all instructions
    vTaskDelay(pdMS_TO_TICKS(500));
    
    TEST_PASS();
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting LCD driver tests");
    
    // Wait a bit for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    UNITY_BEGIN();
    
    RUN_TEST(test_lcd_init_succeeds);
    RUN_TEST(test_lcd_write_single_character);
    RUN_TEST(test_lcd_write_multiple_characters);
    RUN_TEST(test_lcd_write_special_characters);
    RUN_TEST(test_lcd_rapid_writes);
    
    UNITY_END();
    
    ESP_LOGI(TAG, "Tests complete");
}
