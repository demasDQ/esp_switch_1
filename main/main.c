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
#include "app_wifi_ONENET.h"
#include "key_manger.h"
#include "app_key_menu.h"

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
#include "lora_manager.h"

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

    lora_init();
    
    lcd_init();
    wifi_init();

    app_lora_net_init();
    app_wifi_ONENET_init();
}


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
// // 色彩模式循环测试
// static void rgb_test_task(void *arg) {
//     ESP_LOGI("LCD_TEST", "RGB测试任务启动");
    
//     while (1) {

//         display_text_demo(&dev, fx16G);
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
// }
void app_main(void)
{

    
    init();
    
    // 初始化按键管理器
    key_manager_init();
    
    // 初始化按键菜单系统
    app_key_menu_init(&dev, fx16G);
    
    // 启动菜单系统
    app_key_menu_run();
    
    // 保留原有的测试任务（可选）
    // xTaskCreate(rgb_test_task, "rgb_test_task", 1024*2, NULL, configMAX_PRIORITIES-4, NULL);
    

}
