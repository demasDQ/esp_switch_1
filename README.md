# ESP32 Switch 控制系统

基于ESP32的智能开关控制系统，支持4按键LCD菜单、Wi-Fi和LoRa通信功能。

## 功能特点

- 🎛️ 4按键LCD菜单系统（ILI9340显示屏）
- 📶 Wi-Fi网络连接管理
- 📡 LoRa无线通信支持
- ⚙️ FreeRTOS多任务处理
- 🔧 模块化组件设计

## 硬件要求

- ESP32开发板
- ILI9340 LCD显示屏
- 4个按键（上、下、返回、确认）
- （可选）LoRa模块

## 软件环境

- ESP-IDF v5.1或更高版本
- CMake 3.16+
- Python 3.7+

## 构建和烧录

```bash
# 设置目标芯片
idf.py set-target esp32

# 编译项目
idf.py build

# 烧录到设备
idf.py flash

# 监视串口输出
idf.py monitor
```

## 项目结构

```
esp32_switch/
├── components/          # 组件目录
│   ├── ili9340/        # LCD驱动组件
│   ├── key_manager/    # 按键管理组件
│   ├── lora_manager/   # LoRa通信组件
│   └── wifi_manager/   # Wi-Fi管理组件
├── main/               # 主应用程序
├── font/               # 字体文件
└── CMakeLists.txt      # 项目配置文件
```

## 使用说明

详细使用说明请参考 [README_KEY_MENU.md](README_KEY_MENU.md)

## 许可证

MIT License