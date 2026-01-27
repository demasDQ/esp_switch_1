#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

// 按键GPIO配置 - 4个基本按键：上、下、返回、确认
#define KEY_UP_GPIO      GPIO_NUM_14     // 上键
#define KEY_DOWN_GPIO    GPIO_NUM_27     // 下键
#define KEY_BACK_GPIO    GPIO_NUM_19     // 返回键
#define KEY_OK_GPIO      GPIO_NUM_32     // 确认键

// 按键状态
typedef enum {
    KEY_UP,     // 上键
    KEY_DOWN,   // 下键
    KEY_BACK,   // 返回键 (代替原来的左键)
    KEY_OK,     // 确认键
    KEY_NONE    // 无按键
} key_code_t;

// 按键事件回调函数类型
typedef void (*key_event_callback_t)(key_code_t key);

// 初始化按键管理器
void key_manager_init(void);

// 设置按键事件回调
void key_manager_set_callback(key_event_callback_t callback);

// 获取当前按键状态
key_code_t key_manager_get_key(void);

// 检查是否有按键按下
bool key_manager_is_key_pressed(key_code_t key);

#endif // KEY_MANAGER_H