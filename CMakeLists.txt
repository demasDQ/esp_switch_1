# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp_switch_1)

# 启用 SPIFFS 自动构建（从 'font' 目录生成 storage.bin）
spiffs_create_partition_image(storage 
    font 
    FLASH_IN_PROJECT
    PARTITION_OFFSET 0x3E7000  # 与partitions.csv中的偏移地址一致
)
