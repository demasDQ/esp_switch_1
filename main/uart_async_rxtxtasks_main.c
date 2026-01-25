/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "ili9340.h"
#include "fontx.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "app_lora_net.h"

#define TFT_MOSI CONFIG_MOSI_GPIO
#define TFT_SCLK CONFIG_SCLK_GPIO
#define TFT_CS CONFIG_TFT_CS_GPIO
#define GPIO_DC CONFIG_DC_GPIO
#define GPIO_RESET CONFIG_RESET_GPIO
#define GPIO_BL CONFIG_BL_GPIO


TFT_t dev;

static FontxFile fx16G[2];

static const char *TAG = "MAIN";

#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "wifi_manager.h"


bool init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    ESP_LOGI(TAG, "SPIFFS mounted successfully");
    return true;
}

void lcd_init(void) {
    spi_master_init(&dev, TFT_MOSI, TFT_SCLK, TFT_CS, GPIO_DC, GPIO_RESET, GPIO_BL, -1, -1, -1, -1, -1);
    lcdInit(&dev, 0x7735, 128, 160, 2, 1); // ST7735 with 128x160 resolution and configured offsets
    lcdFillScreen(&dev, BLACK);

    // 初始化字体 - 注意：SPIFFS挂载在/spiffs下，路径需要正确设置
    InitFontx(fx16G, "/spiffs/ILGH16XB.FNT", "");

    
}

void init(void) {
    // 初始化SPIFFS文件系统
    if (!init_spiffs()) {
        ESP_LOGE(TAG, "SPIFFS initialization failed, continuing without fonts");
    }

   
    
    lcd_init();
    wifi_init();

    app_lora_net_init();
}

// int sendData(const char* logName, const char* data)
// {
//     const int len = strlen(data);
//     const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
//     ESP_LOGI(logName, "Wrote %d bytes", txBytes);
//     return txBytes;
// }

// static void tx_task(void *arg)
// {
//     static const char *TX_TASK_TAG = "TX_TASK";
//     esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
//     while (1) {
//         sendData(TX_TASK_TAG, "Hello world");
//         vTaskDelay(2000 / portTICK_PERIOD_MS);
//     }
// }

// static void rx_task(void *arg)
// {
//     static const char *RX_TASK_TAG = "RX_TASK";
//     esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
//     uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
//     while (1) {
//         const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
//         if (rxBytes > 0) {
//             data[rxBytes] = 0;
//             ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
//             ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
//         }
//     }
//     free(data);
// }

void display_text_demo(TFT_t *dev, FontxFile *fx) {
    uint16_t color;
    uint8_t ascii[32];
    
    // 清屏
    lcdFillScreen(dev, BLACK);
    
    // 设置字体方向（0, 90, 180, 270度）
    lcdSetFontDirection(dev, DIRECTION0);
    
    // 设置文字颜色
    color = WHITE;
    
    // 绘制字符串
    strcpy((char *)ascii, "Hello World!");
    lcdDrawString(dev, fx, 0, 20, ascii, color);
    
    // 绘制另一行
    strcpy((char *)ascii, "ESP32 ILI9340");
    lcdDrawString(dev, fx, 0, 40, ascii, CYAN);
    
    // 带背景填充的文本
    lcdSetFontFill(dev, RED);
    strcpy((char *)ascii, "Filled Text");
    lcdDrawString(dev, fx, 0, 70, ascii, WHITE);
    lcdUnsetFontFill(dev);
    
    // 带下划线的文本
    lcdSetFontUnderLine(dev, GREEN);
    strcpy((char *)ascii, "Underlined");
    lcdDrawString(dev, fx, 0, 100, ascii, WHITE);
    lcdUnsetFontUnderLine(dev);
}
// 色彩模式循环测试
static void rgb_test_task(void *arg) {
    ESP_LOGI("LCD_TEST", "RGB测试任务启动");
    
    while (1) {

        display_text_demo(&dev, fx16G);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{

    
    init();
    // xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    // xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(rgb_test_task, "rgb_test_task", 1024*2, NULL, configMAX_PRIORITIES-2, NULL);
    

}
