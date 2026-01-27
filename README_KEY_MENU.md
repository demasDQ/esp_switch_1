# 4按键LCD屏幕菜单系统

## 概述
这是一个为ESP32设计的4按键LCD屏幕菜单系统，基于ILI9340/ST7735显示屏和FreeRTOS实时操作系统。系统提供了完整的菜单导航、子菜单支持和功能执行能力。

## 硬件要求
- ESP32开发板
- ILI9340或ST7735 LCD显示屏 (128x160分辨率)
- 5个按键开关 (上、下、左、右、确定)
- 10K电阻（用于按键上拉）

## 按键GPIO连接
| 按键功能 | GPIO引脚 | 物理引脚 |
|----------|----------|----------|
| 上键     | GPIO14   | IO14     |
| 下键     | GPIO27   | IO27     |
| 返回键   | GPIO19   | IO19     |
| 确认键   | GPIO32   | IO32     |

## 软件架构

### 组件结构
```
components/
├── key_manager/          # 按键管理组件
│   ├── key_manger.h     # 按键管理器头文件
│   ├── key_manager.c    # 按键管理器实现
│   └── CMakelists.txt   # 构建配置
└── ili9340/             # 显示驱动组件

main/
├── app_key_menu.h       # 菜单系统头文件
├── app_key_menu.c       # 菜单系统实现
└── main.c              # 主应用程序
```

### 菜单结构
- **主菜单**: Network, System, Display, About
- **Network子菜单**: WiFi Settings, LoRa Settings, Network Test
- **System子菜单**: System Info, Reboot, Shutdown

## 使用说明

### 1. 初始化菜单系统
```c
// 在main.c中初始化
app_key_menu_init(&dev, fx16G);  // dev: TFT设备, fx16G: 字体文件
key_manager_init();              // 初始化按键管理器
app_key_menu_run();              // 启动菜单系统
```

### 2. 添加自定义菜单项
在 `app_key_menu.c` 中修改菜单结构：

```c
// 定义新的菜单项
static menu_item_t custom_items[] = {
    {"Custom Function 1", custom_function_1},
    {"Custom Function 2", custom_function_2}
};

// 定义回调函数
static void custom_function_1(void) {
    // 你的代码
}

static void custom_function_2(void) {
    // 你的代码
}
```

### 3. 按键操作
- **上键/下键**: 菜单项导航
- **左键**: 返回上级菜单
- **右键**: 暂未使用（可自定义）
- **确定键**: 选择菜单项或进入子菜单

## 编译和烧录

### 环境要求
- ESP-IDF v5.1+
- CMake 3.16+

### 编译命令
```bash
idf.py set-target esp32
idf.py build
```

### 烧录命令
```bash
idf.py flash monitor
```

## 自定义配置

### 修改按键GPIO
在 `components/key_manager/key_manger.h` 中修改GPIO定义：
```c
#define KEY_UP_GPIO      GPIO_NUM_14   // 上键
#define KEY_DOWN_GPIO    GPIO_NUM_27   // 下键
#define KEY_BACK_GPIO    GPIO_NUM_19   // 返回键
#define KEY_OK_GPIO      GPIO_NUM_32   // 确认键
```

### 修改菜单结构
在 `main/app_key_menu.c` 中修改菜单项和页面结构。

### 添加新功能
1. 在 `app_key_menu.c` 中添加回调函数
2. 将函数添加到对应的菜单项数组中
3. 重新编译并烧录

## 故障排除

### 常见问题
1. **按键无响应**: 检查GPIO连接和上拉电阻
2. **显示异常**: 检查SPI引脚配置和显示屏初始化
3. **菜单不显示**: 检查字体文件是否正确加载

### 调试信息
系统使用ESP_LOG输出调试信息，可以通过串口监视器查看：
```bash
idf.py monitor
```

## 扩展功能

### 支持的功能
- ✅ 多级菜单导航
- ✅ 按键消抖处理
- ✅ 实时按键响应
- ✅ 自定义菜单项
- ✅ 函数回调执行

### 计划功能
- [ ] 图标支持
- [ ] 动画效果
- [ ] 设置保存
- [ ] 多语言支持

## 许可证
MIT License

## 技术支持
如有问题，请提交Issue或联系开发团队。