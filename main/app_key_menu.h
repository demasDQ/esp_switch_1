#ifndef APP_KEY_MENU_H
#define APP_KEY_MENU_H

#include "ili9340.h"
#include "fontx.h"

// 菜单项结构
typedef struct {
    const char* text;
    void (*action)(void);
} menu_item_t;

// 菜单页结构
typedef struct menu_page {
    const char* title;
    menu_item_t* items;
    int item_count;
    struct menu_page* parent;
    struct menu_page** children;
} menu_page_t;

// 菜单系统初始化
void app_key_menu_init(TFT_t* dev, FontxFile* font);

// 菜单系统运行
void app_key_menu_run(void);

// 菜单系统停止
void app_key_menu_stop(void);

// 显示主菜单
void show_main_menu(void);

// 显示子菜单
void show_sub_menu(menu_page_t* sub_menu);

#endif // APP_KEY_MENU_H