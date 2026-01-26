#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include "esp_err.h"

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

#define LORA_M0_PIN GPIO_NUM_25
#define LORA_M1_PIN GPIO_NUM_26
#define LORA_GPIO_OUTPUT_PIN_SEL ((1ULL<<LORA_M0_PIN)|(1ULL<<LORA_M1_PIN))

#define LORA_M0(a)  gpio_set_level(LORA_M0_PIN, a)
#define LORA_M1(a)  gpio_set_level(LORA_M1_PIN, a)
#define READ_AUX    gpio_get_level(LORA_AUX_PIN)



void lora_init(void);

// UART发送函数声明
esp_err_t uart_send_data(const char* data, size_t length);
esp_err_t uart_send_at_command(const char* at_command);

// 其他已有函数声明
void uart_init(void);
void gpio_init(void);
void app_lora_init(void);


// LoRa模式枚举
typedef enum lora_mode {
    lora_mode_transfer,  // 传输模式
    lora_mode_wor,       // WOR模式（Wake On Radio）
    lora_mode_cfg,       // 配置模式
    lora_mode_sleep      // 睡眠模式
}lora_mode_t;

uint8_t lora_mode_config(lora_mode_t mode);
#endif // LORA_MANAGER_H
