#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/uart.h"

// 帧状态枚举类型定义
typedef enum {
    FRAME_STATE_IDLE,
    FRAME_STATE_HEADER,
    FRAME_STATE_DATA,
    FRAME_STATE_CHECKSUM,
    FRAME_STATE_END
} frame_state_t;

// 帧解析器结构体定义
typedef struct {
    uint8_t buffer[256];
    uint16_t length;
    frame_state_t state;
    uint8_t expected_len;
    uint8_t current_data_len;  
    uint8_t calculated_checksum; 
} frame_parser_t;

typedef enum {
    CMD_QUERY,
    CMD_QUERY_RESPONSE_DATA
} command_type_t;

// 消息结构体定义
typedef struct {
    uint8_t slave_id;
    command_type_t command;
    uint8_t data[32];
    uint8_t data_len;
} frame_message_t;




#define START_BYTE 0xAA
#define END_BYTE  0X55

#define SLAVE_1_ID 0x01
#define SLAVE_2_ID 0x02

// 事件组位定义
#define RESPONSE_SLAVE_1_BIT (1 << 0)  // 收到从机1的响应
#define RESPONSE_SLAVE_2_BIT (1 << 1)  // 收到从机2的响应


void app_lora_net_init(void);