# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp_switch_1)

set(PARTITION_TABLE partitions.csv)

# 注释掉spiffs_create_partition_image，让ESP-IDF自动处理
# spiffs_create_partition_image(storage 
#     font 
#     FLASH_IN_PROJECT
#     PARTITION_OFFSET 0x3E7000  # 与partitions.csv中的偏移地址一致
# )