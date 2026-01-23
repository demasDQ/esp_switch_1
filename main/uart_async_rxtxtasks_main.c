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

#define TFT_MOSI CONFIG_MOSI_GPIO
#define TFT_SCLK CONFIG_SCLK_GPIO
#define TFT_CS CONFIG_TFT_CS_GPIO
#define GPIO_DC CONFIG_DC_GPIO
#define GPIO_RESET CONFIG_RESET_GPIO
#define GPIO_BL CONFIG_BL_GPIO

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

TFT_t dev;

static FontxFile fx16G[2];

static const char *TAG = "MAIN";

#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "wifi_manager.h"
static const int RX_BUF_SIZE = 1024;

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

void init_lcd(void) {
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

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    init_lcd();
    wifi_init();
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
// RGB颜色测试函数
// static void test_basic_colors(void) {
//     ESP_LOGI("LCD_TEST", "=== 基础颜色测试 ===");
//     const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, BLACK, YELLOW, CYAN, PURPLE, GRAY};
//     const char* color_names[] = {"红色", "绿色", "蓝色", "白色", "黑色", "黄色", "青色", "紫色", "灰色"};
    
//     for (int i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
//         ESP_LOGI("LCD_TEST", "显示: %s", color_names[i]);
//         lcdFillScreen(&dev, colors[i]);
//         vTaskDelay(pdMS_TO_TICKS(1500));
//     }
// }

// // RGB条纹测试函数
// static void test_rgb_stripes(void) {
//     ESP_LOGI("LCD_TEST", "=== RGB条纹测试 ===");
    
//     // 竖条纹测试
//     ESP_LOGI("LCD_TEST", "竖条纹测试");
//     for (int repeat = 0; repeat < 2; repeat++) {
//         for (int stripe = 0; stripe < 3; stripe++) {
//             lcdFillScreen(&dev, BLACK);
//             uint16_t stripe_width = dev._width / 3;
//             uint16_t x_start = stripe * stripe_width;
//             uint16_t x_end = (stripe + 1) * stripe_width - 1;
            
//             lcdDrawFillRect(&dev, x_start, 0, x_end, dev._height - 1, 
//                            (stripe == 0) ? RED : ((stripe == 1) ? GREEN : BLUE));
//             vTaskDelay(pdMS_TO_TICKS(800));
//         }
//     }
    
//     // 横条纹测试
//     ESP_LOGI("LCD_TEST", "横条纹测试");
//     for (int repeat = 0; repeat < 2; repeat++) {
//         for (int stripe = 0; stripe < 3; stripe++) {
//             lcdFillScreen(&dev, BLACK);
//             uint16_t stripe_height = dev._height / 3;
//             uint16_t y_start = stripe * stripe_height;
//             uint16_t y_end = (stripe + 1) * stripe_height - 1;
            
//             lcdDrawFillRect(&dev, 0, y_start, dev._width - 1, y_end,
//                            (stripe == 0) ? RED : ((stripe == 1) ? GREEN : BLUE));
//             vTaskDelay(pdMS_TO_TICKS(800));
//         }
//     }
// }

// // 渐变测试函数
// static void test_gradient(void) {
//     ESP_LOGI("LCD_TEST", "=== 渐变效果测试 ===");
    
//     // 红色渐变
//     ESP_LOGI("LCD_TEST", "红色渐变");
//     for (int intensity = 0; intensity <= 255; intensity += 8) {
//         uint16_t color = rgb565(intensity, 0, 0);
//         lcdFillScreen(&dev, color);
//         vTaskDelay(pdMS_TO_TICKS(30));
//     }
    
//     // 绿色渐变
//     ESP_LOGI("LCD_TEST", "绿色渐变");
//     for (int intensity = 0; intensity <= 255; intensity += 8) {
//         uint16_t color = rgb565(0, intensity, 0);
//         lcdFillScreen(&dev, color);
//         vTaskDelay(pdMS_TO_TICKS(30));
//     }
    
//     // 蓝色渐变
//     ESP_LOGI("LCD_TEST", "蓝色渐变");
//     for (int intensity = 0; intensity <= 255; intensity += 8) {
//         uint16_t color = rgb565(0, 0, intensity);
//         lcdFillScreen(&dev, color);
//         vTaskDelay(pdMS_TO_TICKS(30));
//     }
    
//     // 白色渐变
//     ESP_LOGI("LCD_TEST", "白色渐变");
//     for (int intensity = 0; intensity <= 255; intensity += 8) {
//         uint16_t color = rgb565(intensity, intensity, intensity);
//         lcdFillScreen(&dev, color);
//         vTaskDelay(pdMS_TO_TICKS(30));
//     }
// }

// // 反色测试函数
// static void test_inverse_colors(void) {
//     ESP_LOGI("LCD_TEST", "=== 反色对比测试 ===");
    
//     const uint16_t pairs[][2] = {
//         {BLACK, WHITE},
//         {RED, CYAN},
//         {GREEN, PURPLE},
//         {BLUE, YELLOW}
//     };
    
//     for (int i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++) {
//         // 左半屏
//         lcdDrawFillRect(&dev, 0, 0, dev._width / 2 - 1, dev._height - 1, pairs[i][0]);
//         // 右半屏
//         lcdDrawFillRect(&dev, dev._width / 2, 0, dev._width - 1, dev._height - 1, pairs[i][1]);
//         vTaskDelay(pdMS_TO_TICKS(2000));
//     }
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
        // // 模式1: 基础颜色测试
        // test_basic_colors();
        
        // // 模式2: RGB条纹测试
        // test_rgb_stripes();
        
        // // 模式3: 渐变效果测试
        // test_gradient();
        
        // // 模式4: 反色对比测试
        // test_inverse_colors();
        
        // ESP_LOGI("LCD_TEST", "=== 完成一轮测试，开始下一轮 ===");
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
