#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora_manager.h"

static const int RX_BUF_SIZE = 1024;

uint8_t lora_mode_config(lora_mode_t mode){
    switch(mode){
        case lora_mode_transfer: LORA_M0(0); LORA_M1(0); break;
        case lora_mode_wor:      LORA_M0(1); LORA_M1(0); break;
        case lora_mode_cfg:      LORA_M0(0); LORA_M1(1); break;
        case lora_mode_sleep:    LORA_M0(1); LORA_M1(1); break;
        default: return 1;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return 0;
}

void uart_init(void) {

 const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


void gpio_init(void) {

        gpio_config_t io_conf = {
            .pin_bit_mask = LORA_GPIO_OUTPUT_PIN_SEL,  // GPIO输出引脚选择
            .mode = GPIO_MODE_OUTPUT,                   // 输出模式
            .pull_up_en = GPIO_PULLUP_DISABLE,          // 禁止上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE,      // 禁止下拉
            .intr_type = GPIO_INTR_DISABLE              // 禁止中断
        };
        gpio_config(&io_conf);                         // 应用GPIO配置
}

// 向UART发送数据函数
esp_err_t uart_send_data(const char* data, size_t length)
{
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const int txBytes = uart_write_bytes(UART_NUM_2, data, length);
    if (txBytes < 0) {
        return ESP_FAIL;
    }
    
    // 等待发送完成
    return uart_wait_tx_done(UART_NUM_2, 1000 / portTICK_PERIOD_MS);
}

// 等待OK响应的函数
static esp_err_t wait_for_ok(int timeout_ms)
{
    uint8_t buffer[64];
    int total_time = 0;
    const int tick_delay = 10;
    
    while (total_time < timeout_ms) {
        int rxBytes = uart_read_bytes(UART_NUM_2, buffer, sizeof(buffer) - 1, tick_delay / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            buffer[rxBytes] = '\0';
            ESP_LOGI("UART_RX", "Received: %s", buffer);
            
            // 检查是否包含"=OK"（根据模块文档要求的响应格式）
            if (strstr((char*)buffer, "=OK") != NULL) {
                return ESP_OK;
            }
        }
        total_time += tick_delay;
        vTaskDelay(tick_delay / portTICK_PERIOD_MS);
    }
    
    ESP_LOGE("UART_RX", "Timeout waiting for OK response");
    return ESP_ERR_TIMEOUT;
}

// 发送AT指令的封装函数
esp_err_t uart_send_at_command(const char* at_command)
{
    if (at_command == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI("UART_TX", "Sending AT command: %s", at_command);
    return uart_send_data(at_command, strlen(at_command));
}





void lora_init(void)
{
    uart_init();   // 初始化串口
    gpio_init();   // 初始化GPIO


    // 切换到配置模式
    ESP_LOGI("LORA", "Switching to configuration mode");
    lora_mode_config(lora_mode_cfg);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // LoRa模块AT指令配置序列
    const struct {
        const char* command;
        const char* description;
        int timeout_ms;
    } lora_config_commands[] = {
        {"AT+UART=3,0\r\n",    "Set UART: 9600bps, no parity", 200},
        {"AT+ADDR=0\r\n",      "Set address: 0x0000", 200},
        {"AT+NETID=0\r\n",     "Set network ID: 0", 200},
        {"AT+CHANNEL=0\r\n",   "Set channel: 0", 200},
        {"AT+TRANS=1\r\n",     "Set transmission mode: point-to-point", 200},
        {"AT+URXT=3\r\n",      "Set UART frame timeout: 3 bytes", 200}
    };

    // 执行所有配置命令
    for (int i = 0; i < sizeof(lora_config_commands) / sizeof(lora_config_commands[0]); i++) {
        ESP_LOGI("LORA", "Configuring: %s", lora_config_commands[i].description);
        uart_send_at_command(lora_config_commands[i].command);
        
        esp_err_t ret = wait_for_ok(lora_config_commands[i].timeout_ms);
        if (ret != ESP_OK) {
            ESP_LOGE("LORA", "Failed to configure: %s", lora_config_commands[i].description);
        } else {
            ESP_LOGI("LORA", "Successfully configured: %s", lora_config_commands[i].description);
        }
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);  // 配置完成后延时

    // 切换到传输模式
    lora_mode_config(lora_mode_transfer);
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // 统一使用vTaskDelay

    ESP_LOGI("LORA", "Lora initialization complete.");
}