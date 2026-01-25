#ifndef _APP_LORA_NET_H_
#define _APP_LORA_NET_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

// 最大帧长度（需覆盖：START + ID + CMD + LEN + DATA + CHECKSUM + END）
#define MAX_FRAME_SIZE 256

// 帧解析状态机状态
typedef enum {
    FRAME_STATE_IDLE,
    FRAME_STATE_HEADER,
    FRAME_STATE_DATA,
    FRAME_STATE_CHECKSUM,
    FRAME_STATE_END
} frame_state_t;

// 帧解析器上下文
typedef struct {
    uint8_t buffer[MAX_FRAME_SIZE];
    uint16_t length;
    frame_state_t state;
    uint8_t expected_len;          // 保留字段（当前未使用）
    uint8_t current_data_len;      // 从帧头中解析出的数据长度
    uint8_t calculated_checksum;   // 保留字段（当前未使用）
} frame_parser_t;

// 支持的命令类型
typedef enum {
    CMD_QUERY,
    CMD_QUERY_RESPONSE_DATA
} command_type_t;

// 队列中传递的消息结构
typedef struct {
    uint8_t slave_id;
    command_type_t command;
    uint8_t data[32];              // 最大数据负载
    uint8_t data_len;              // 实际数据长度
} frame_message_t;

// 帧起始与结束标志
#define START_BYTE 0xAA
#define END_BYTE   0x55

// 从机地址定义
#define SLAVE_1_ID 0x01
#define SLAVE_2_ID 0x02

// 事件组位掩码
#define RESPONSE_SLAVE_1_BIT (1 << 0)  // 从机1响应到位
#define RESPONSE_SLAVE_2_BIT (1 << 1)  // 从机2响应到位

// 应用初始化入口
void app_lora_net_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_LORA_NET_H_ */