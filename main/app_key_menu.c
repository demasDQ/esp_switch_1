#include "app_key_menu.h"
#include "key_manger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "APP_KEY_MENU";

static TFT_t* lcd_dev = NULL;
static FontxFile* lcd_font = NULL;
static menu_page_t* current_menu = NULL;
static int current_selection = 0;
static TaskHandle_t menu_task_handle = NULL;
static bool menu_running = false;

// 示例菜单项的回调函数
static void action_wifi_settings(void) {
    ESP_LOGI(TAG, "WiFi settings selected");
    lcdFillScreen(lcd_dev, BLUE);
    lcdDrawString(lcd_dev, lcd_font, 10, 50, (uint8_t*)"WiFi Settings", WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void action_lora_settings(void) {
    ESP_LOGI(TAG, "LoRa settings selected");
    lcdFillScreen(lcd_dev, GREEN);
    lcdDrawString(lcd_dev, lcd_font, 10, 50, (uint8_t*)"LoRa Settings", WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void action_system_info(void) {
    ESP_LOGI(TAG, "System info selected");
    lcdFillScreen(lcd_dev, PURPLE);
    lcdDrawString(lcd_dev, lcd_font, 10, 50, (uint8_t*)"System Info", WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void action_network_test(void) {
    ESP_LOGI(TAG, "Network test selected");
    lcdFillScreen(lcd_dev, YELLOW);
    lcdDrawString(lcd_dev, lcd_font, 10, 50, (uint8_t*)"Network Test", BLACK);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

// 补水控制功能
static bool watering_enabled = false;
static void action_watering_control(void) {
    watering_enabled = !watering_enabled;
    ESP_LOGI(TAG, "Watering %s", watering_enabled ? "ENABLED" : "DISABLED");
    
    lcdFillScreen(lcd_dev, watering_enabled ? GREEN : RED);
    lcdDrawString(lcd_dev, lcd_font, 10, 30, (uint8_t*)"Watering Control", WHITE);
    lcdDrawString(lcd_dev, lcd_font, 10, 60, (uint8_t*)"Status:", WHITE);
    lcdDrawString(lcd_dev, lcd_font, 70, 60, 
                 (uint8_t*)(watering_enabled ? "ON " : "OFF"), WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

// 补光控制功能
static bool lighting_enabled = false;
static void action_lighting_control(void) {
    lighting_enabled = !lighting_enabled;
    ESP_LOGI(TAG, "Lighting %s", lighting_enabled ? "ENABLED" : "DISABLED");
    
    lcdFillScreen(lcd_dev, lighting_enabled ? YELLOW : BLUE);
    lcdDrawString(lcd_dev, lcd_font, 10, 30, (uint8_t*)"Lighting Control", WHITE);
    lcdDrawString(lcd_dev, lcd_font, 10, 60, (uint8_t*)"Status:", WHITE);
    lcdDrawString(lcd_dev, lcd_font, 70, 60, 
                 (uint8_t*)(lighting_enabled ? "ON " : "OFF"), WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

// 子菜单项
static menu_item_t network_items[] = {
    {"WiFi Settings", action_wifi_settings},
    {"LoRa Settings", action_lora_settings},
    {"Network Test", action_network_test}
};

static menu_item_t system_items[] = {
    {"System Info", action_system_info},
    {"Reboot", NULL},
    {"Shutdown", NULL}
};

// 植物养护菜单项，注册了补水补光的回调函数，用以控制补水补光设备。
static menu_item_t plant_care_items[] = {
    {"Watering Control", action_watering_control},
    {"Lighting Control", action_lighting_control},
    {"Back", NULL}
};

// 子菜单页面
static menu_page_t network_menu = {
    .title = "Network",
    .items = network_items,
    .item_count = 3,
    .parent = NULL,
    .children = NULL
};

static menu_page_t system_menu = {
    .title = "System",
    .items = system_items,
    .item_count = 3,
    .parent = NULL,
    .children = NULL
};

// 植物养护子菜单
static menu_page_t plant_care_menu = {
    .title = "Plant Care",
    .items = plant_care_items,
    .item_count = 3,
    .parent = NULL,
    .children = NULL
};

// 主菜单项
static menu_item_t main_items[] = {
    {"Network", NULL},
    {"System", NULL},
    {"Plant Care", NULL},  // 新增植物养护菜单
    {"Display", NULL},
    {"About", NULL}
};

// 主菜单页面
static menu_page_t main_menu = {
    .title = "Main Menu",
    .items = main_items,
    .item_count = 5,
    .parent = NULL,
    .children = (menu_page_t*[]){&network_menu, &system_menu, &plant_care_menu, NULL, NULL}
};

// 初始化菜单关系
static void init_menu_hierarchy(void) {
    network_menu.parent = &main_menu;
    system_menu.parent = &main_menu;
    plant_care_menu.parent = &main_menu;
}

// 绘制菜单
static void draw_menu(menu_page_t* menu, int selection) {
    lcdFillScreen(lcd_dev, BLACK);
    
    // 绘制标题
    lcdDrawString(lcd_dev, lcd_font, 10, 10, (uint8_t*)menu->title, WHITE);
    lcdDrawLine(lcd_dev, 0, 30, 128, 30, GRAY);
    
    // 绘制菜单项
    for (int i = 0; i < menu->item_count; i++) {
        uint16_t color = (i == selection) ? YELLOW : WHITE;
        uint16_t bg_color = (i == selection) ? BLUE : BLACK;
        
        // 绘制选中背景
        lcdDrawFillRect(lcd_dev, 0, 35 + i * 25, 128, 35 + i * 25 + 20, bg_color);
        
        // 绘制菜单文本
        lcdDrawString(lcd_dev, lcd_font, 15, 40 + i * 25, (uint8_t*)menu->items[i].text, color);
        
        // 绘制选中标记
        if (i == selection) {
            lcdDrawString(lcd_dev, lcd_font, 5, 40 + i * 25, (uint8_t*)">", YELLOW);
        }
    }
}

// 按键事件处理
static void key_event_handler(key_code_t key) {
    if (!menu_running) return;

    switch (key) {
        case KEY_UP:
            current_selection = (current_selection - 1 + current_menu->item_count) % current_menu->item_count;
            draw_menu(current_menu, current_selection);
            break;
            
        case KEY_DOWN:
            current_selection = (current_selection + 1) % current_menu->item_count;
            draw_menu(current_menu, current_selection);
            break;
            
        case KEY_OK:
            if (current_menu->items[current_selection].action) {
                // 执行菜单项动作
                current_menu->items[current_selection].action();
                draw_menu(current_menu, current_selection);
            } else if (current_menu->children && current_menu->children[current_selection]) {
                // 进入子菜单
                current_menu = current_menu->children[current_selection];
                current_selection = 0;
                draw_menu(current_menu, current_selection);
            }
            break;
            
        case KEY_BACK:  // 原来为KEY_LEFT，现在是KEY_BACK
            // 返回上一级菜单
            if (current_menu->parent) {
                current_menu = current_menu->parent;
                current_selection = 0;
                draw_menu(current_menu, current_selection);
            }
            break;
            
        default:
            break;
    }
}

// 菜单任务
static void menu_task(void* arg) {
    ESP_LOGI(TAG, "Menu task started");
    
    // 初始化菜单层级
    init_menu_hierarchy();
    current_menu = &main_menu;
    
    // 绘制初始菜单
    draw_menu(current_menu, current_selection);
    
    // 注册按键回调  
    key_manager_set_callback(key_event_handler);
    
    while (menu_running) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "Menu task stopped");
    vTaskDelete(NULL);
}

void app_key_menu_init(TFT_t* dev, FontxFile* font) {
    lcd_dev = dev;
    lcd_font = font;
    ESP_LOGI(TAG, "App key menu initialized");
}

void app_key_menu_run(void) {
    if (menu_running) return;
    
    menu_running = true;
    xTaskCreate(menu_task, "menu_task", 4096, NULL, 5, &menu_task_handle);
}

void app_key_menu_stop(void) {
    menu_running = false;
    if (menu_task_handle) {
        vTaskDelete(menu_task_handle);
        menu_task_handle = NULL;
    }
}

void show_main_menu(void) {
    current_menu = &main_menu;
    current_selection = 0;
    draw_menu(current_menu, current_selection);
}

void show_sub_menu(menu_page_t* sub_menu) {
    if (sub_menu) {
        current_menu = sub_menu;
        current_selection = 0;
        draw_menu(current_menu, current_selection);
    }
}