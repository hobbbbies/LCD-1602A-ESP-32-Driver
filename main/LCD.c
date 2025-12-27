#include "LCD.h"
#include "esp_log.h"

// Debug control: Set to 1 to enable debug logs, 0 to disable
#define LCD_DEBUG 1

#if LCD_DEBUG
    static const char* TAG = "LCD";
    #define LCD_LOGD(fmt, ...) ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
    #define LCD_LOGI(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#else
    #define LCD_LOGD(fmt, ...)
    #define LCD_LOGI(fmt, ...)
#endif

// LCD GPIO Pin Definitions
#define LCD_D4_PIN  GPIO_NUM_4
#define LCD_D5_PIN  GPIO_NUM_5
#define LCD_D6_PIN  GPIO_NUM_13
#define LCD_D7_PIN  GPIO_NUM_18
#define LCD_EN_PIN  GPIO_NUM_19
#define LCD_RS_PIN  GPIO_NUM_21

static esp_timer_handle_t timer;
static QueueHandle_t instruction_queue;

typedef enum {
    LCD_IDLE,
    LCD_SENDING_HIGH,
    LCD_SENDING_LOW,
    LCD_EXECUTING
} lcd_state_t;

typedef struct {
    uint8_t current_instruction; 
    uint16_t current_exec_time;
} lcd_instruction;

typedef struct {
    lcd_state_t state;
    lcd_instruction current;
} lcd_ctx_t;

static lcd_ctx_t lcd_context;

static void lcd_pulse_e() {
    gpio_set_level(LCD_EN_PIN, 1);
    ets_delay_us(1);
    gpio_set_level(LCD_EN_PIN, 0);
}

static void lcd_set_pins(uint8_t byte) {
    gpio_set_level(LCD_D4_PIN, byte & 1);
    gpio_set_level(LCD_D5_PIN, (byte >> 1) & 1);
    gpio_set_level(LCD_D6_PIN, (byte >> 2) & 1);
    gpio_set_level(LCD_D7_PIN, (byte >> 3) & 1);
}

static void lcd_send_nibble(uint8_t byte, bool is_data) {
    LCD_LOGD("Sending nibble: 0x%X, is_data=%d", byte & 0x0F, is_data);
    gpio_set_level(LCD_RS_PIN, is_data);
    // LOW nibble 
    lcd_set_pins(byte & 0x0F);
    lcd_pulse_e();
}

void execute_next_lcd_instruction() {
    if (lcd_context.state != LCD_IDLE) {
        LCD_LOGD("Skipping execute_next: state=%d (not IDLE)", lcd_context.state);
        return;
    }

    lcd_instruction instruction;
    if (xQueueReceive(instruction_queue, &instruction, 0) == pdPASS) {
        LCD_LOGD("Dequeued instruction: 0x%02X, exec_time=%d us", instruction.current_instruction, instruction.current_exec_time);
        // call esp_timer and set state     
        lcd_context.state = LCD_SENDING_HIGH;
        lcd_context.current.current_instruction = instruction.current_instruction;
        lcd_context.current.current_exec_time = instruction.current_exec_time;
        esp_timer_start_once(timer, 1);
    } else {
        LCD_LOGD("Queue empty, no instruction to execute");
    }
}

// state machine 
void state_timer_callback(void* args) {
    LCD_LOGD("Timer callback: state=%d", lcd_context.state);
    switch(lcd_context.state) {
        case LCD_SENDING_HIGH:
            LCD_LOGD("State: SENDING_HIGH -> SENDING_LOW");
            lcd_send_nibble(lcd_context.current.current_instruction >> 4, 0);
            lcd_context.state = LCD_SENDING_LOW;
            esp_timer_start_once(timer, 1);
            break;
        case LCD_SENDING_LOW:
            LCD_LOGD("State: SENDING_LOW -> EXECUTING");
            lcd_send_nibble(lcd_context.current.current_instruction, 0);
            lcd_context.state = LCD_EXECUTING;
            esp_timer_start_once(timer, lcd_context.current.current_exec_time);
            break;
        case LCD_EXECUTING:
            LCD_LOGD("State: EXECUTING -> IDLE");
            lcd_context.state = LCD_IDLE;
            execute_next_lcd_instruction();
            break;
        default: 
            break;
    }
}

void send_lcd_instruction(uint8_t current_instruction, uint16_t current_exec_time) {
    LCD_LOGD("Queuing instruction: 0x%02X, exec_time=%d us", current_instruction, current_exec_time);
    lcd_instruction instruction = {.current_instruction = current_instruction, .current_exec_time = current_exec_time};
    xQueueSendToBack(instruction_queue, &instruction, portMAX_DELAY);
}

void lcd_bootstrap() {
    LCD_LOGI("Starting LCD bootstrap sequence");
    ets_delay_us(15500);
    LCD_LOGD("Bootstrap: Sending 0x3 (1/3)");
    lcd_send_nibble(0b0011, 0);
    ets_delay_us(4100);
    LCD_LOGD("Bootstrap: Sending 0x3 (2/3)");
    lcd_send_nibble(0b0011, 0);
    ets_delay_us(100);
    LCD_LOGD("Bootstrap: Sending 0x3 (3/3)");
    lcd_send_nibble(0b0011, 0);

    // enable 4 bit mode from 8 bit mode 
    LCD_LOGD("Bootstrap: Switching to 4-bit mode");
    lcd_send_nibble(0b0010, 0);
    ets_delay_us(40);
    
    // function set
    send_lcd_instruction(0b00101000, 39);

    // display off
    send_lcd_instruction(0b00001000, 39);

    // display clear
    send_lcd_instruction(0b00000001, 1530);

    // entry mode set
    send_lcd_instruction(0b00000110, 39);

    execute_next_lcd_instruction();
}

void lcd_init() {
    LCD_LOGI("Initializing LCD driver");
    esp_timer_create_args_t timer_args = {
        .name = "state_timer",
        .callback = &state_timer_callback,
        .dispatch_method = ESP_TIMER_TASK
    };
    esp_timer_create(&timer_args, &timer);
    LCD_LOGD("Timer created");

    instruction_queue = xQueueCreate(10, sizeof(lcd_instruction));
    LCD_LOGD("Instruction queue created");

    gpio_config_t io_cfg = {};
    io_cfg.mode = GPIO_MODE_OUTPUT;
    io_cfg.intr_type = GPIO_INTR_DISABLE;
    io_cfg.pin_bit_mask = (1ULL << LCD_D4_PIN) | (1ULL << LCD_D5_PIN) | (1ULL << LCD_D6_PIN) | (1ULL << LCD_D7_PIN) | (1ULL << LCD_EN_PIN) | (1ULL << LCD_RS_PIN);
    io_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_cfg);

    lcd_bootstrap();

    LCD_LOGI("Sending display ON command");
    send_lcd_instruction(0x0C, 39);
    execute_next_lcd_instruction();
    LCD_LOGI("LCD initialization complete");
}
