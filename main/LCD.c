#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

// LCD GPIO Pin Definitions
#define LCD_D4_PIN  GPIO_NUM_4
#define LCD_D5_PIN  GPIO_NUM_5
#define LCD_D6_PIN  GPIO_NUM_13
#define LCD_D7_PIN  GPIO_NUM_18
#define LCD_EN_PIN  GPIO_NUM_19
#define LCD_RS_PIN  GPIO_NUM_21

esp_timer_handle_t timer;

typedef enum {
    LCD_IDLE,
    LCD_SENDING_HIGH,
    LCD_SENDING_LOW
} lcd_state_t;

typedef struct {
    lcd_state_t state;
    uint8_t current_instruction; 
    uint16_t current_exec_time;
} lcd_ctx_t;

static lcd_ctx_t lcd_context;


static void lcd_pulse_e() {
    gpio_set_level(LCD_EN_PIN, 0);
    gpio_set_level(LCD_EN_PIN, 1);
}

static void lcd_set_pins(uint8_t byte) {
    gpio_set_level(LCD_D4_PIN, byte & 1);
    gpio_set_level(LCD_D5_PIN, (byte >> 1) & 1);
    gpio_set_level(LCD_D6_PIN, (byte >> 2) & 1);
    gpio_set_level(LCD_D7_PIN, (byte >> 3) & 1);
}

static void lcd_send_nibble(uint8_t byte, bool is_data) {
    gpio_set_level(LCD_RS_PIN, is_data);
    // LOW nibble 
    lcd_set_pins(byte & 0x0F);
    lcd_pulse_e();
}

// state machine 
void state_timer_callback() {
    switch(lcd_context.state) {
        case LCD_SENDING_HIGH:
            lcd_send_nibble(lcd_context.current_instruction >> 4, 0);
            lcd_context.state = LCD_SENDING_LOW;
            esp_timer_start_once(timer, 1);
            break;
        case LCD_SENDING_LOW:
            lcd_send_nibble(lcd_context.current_instruction, 0);
            lcd_context.state = LCD_IDLE;
            esp_timer_start_once(timer, lcd_context.current_exec_time);
            break;
        default: 
            break;
    }
}

void send_lcd_instruction(uint8_t current_instruction, uint16_t current_exec_time) {
    // call esp_timer and set state     
    lcd_context.state = LCD_SENDING_HIGH;
    lcd_context.current_instruction = current_instruction;
    lcd_context.current_exec_time = current_exec_time;
    esp_timer_start_once(timer, 1);
}

void lcd_bootstrap() {
    esp_timer_start_once(timer, 15500);
    lcd_send_nibble(0b00000011, 0);
    esp_timer_start_once(timer, 4100);
    lcd_send_nibble(0b00000011, 0);
    esp_timer_start_once(timer, 100);
    // enable 4 bit mode from 8 bit mode 
    lcd_send_nibble(0b00000010, 0);
    lcd_send_nibble(0b0010, 0);
    lcd_send_nibble(0b1000, 0);

    // display off
    lcd_send_nibble(0b0000, 0);
    lcd_send_nibble(0b1000, 0);

    // display off
    lcd_send_nibble(0b0000, 0);
    lcd_send_nibble(0b0001, 0);

    // entry mode set
    lcd_send_nibble(0b0000, 0);
    lcd_send_nibble(0b0110, 0);
}

void lcd_init() {
    esp_timer_create_args_t timer_args = {
        .name = "state_timer",
        .callback = &state_timer_callback,
        .dispatch_method = ESP_TIMER_TASK
    };

    esp_timer_create(&timer_args, &timer);

    gpio_config_t io_cfg = {};
    io_cfg.mode = GPIO_MODE_OUTPUT;
    io_cfg.intr_type = GPIO_INTR_DISABLE;
    io_cfg.pin_bit_mask = (1ULL << LCD_D4_PIN) | (1ULL << LCD_D5_PIN) | (1ULL << LCD_D6_PIN) | (1ULL << LCD_D7_PIN) | (1ULL << LCD_EN_PIN) | (1ULL << LCD_RS_PIN);
    io_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_cfg);

    lcd_send_nibble(0b00001010, 0);

    lcd_send_nibble(0b00001111, 0);
}

void lcd_write() {

}