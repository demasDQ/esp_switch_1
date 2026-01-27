#include "key_manger.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "KEY_MANAGER";

static key_event_callback_t key_callback = NULL;
static QueueHandle_t key_queue = NULL;

// 按键引脚配置 - 只有4个按键：上、下、返回、确认
static const gpio_num_t key_gpios[] = {
    KEY_UP_GPIO,    // KEY_UP (0)
    KEY_DOWN_GPIO,  // KEY_DOWN (1)  
    KEY_BACK_GPIO,  // KEY_BACK (2) - 原来的LEFT键现在用作返回
    KEY_OK_GPIO     // KEY_OK (3)
};

// GPIO中断处理函数
static void IRAM_ATTR key_isr_handler(void* arg) {
    key_code_t key = (key_code_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // 发送按键事件到队列
    xQueueSendFromISR(key_queue, &key, &xHigherPriorityTaskWoken);
    
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// 按键处理任务
static void key_task(void* arg) {
    key_code_t key;
    
    while (1) {
        if (xQueueReceive(key_queue, &key, portMAX_DELAY)) {
            // 去抖动延时
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // 检查按键是否仍然按下
            if (!gpio_get_level(key_gpios[key])) {
                ESP_LOGI(TAG, "Key pressed: %d", key);
                
                // 调用回调函数
                if (key_callback) {
                    key_callback(key);
                }
            }
        }
    }
}

void key_manager_init(void) {
    // 创建按键队列
    key_queue = xQueueCreate(10, sizeof(key_code_t));
    
    // 配置按键GPIO为输入模式，上拉
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << KEY_UP_GPIO) | 
                       (1ULL << KEY_DOWN_GPIO) | 
                       (1ULL << KEY_BACK_GPIO) | 
                       (1ULL << KEY_OK_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
    
    // 安装GPIO中断服务
    gpio_install_isr_service(0);
    
    // 为每个按键注册中断处理函数
    for (int i = 0; i < sizeof(key_gpios) / sizeof(key_gpios[0]); i++) {
        gpio_isr_handler_add(key_gpios[i], key_isr_handler, (void*)i);
    }
    
    // 创建按键处理任务
    xTaskCreate(key_task, "key_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Key manager initialized");
}

void key_manager_set_callback(key_event_callback_t callback) {
    key_callback = callback;
}

key_code_t key_manager_get_key(void) {
    for (int i = 0; i < sizeof(key_gpios) / sizeof(key_gpios[0]); i++) {
        if (!gpio_get_level(key_gpios[i])) {
            return (key_code_t)i;
        }
    }
    return KEY_NONE;
}

bool key_manager_is_key_pressed(key_code_t key) {
    if (key >= KEY_UP && key <= KEY_OK) {
        return !gpio_get_level(key_gpios[key]);
    }
    return false;
}