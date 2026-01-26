#include "app_lora_net.h"

// 计算校验和的辅助函数，用于计算校验和
static uint8_t calculate_checksum(const uint8_t *data, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}
// 发送协议帧到某个从节点的函数
static int send_protocol_frame(uint8_t slave_id, command_type_t command, const uint8_t *data, uint8_t data_len)
{
    uint8_t frame[MAX_FRAME_SIZE];
    uint8_t frame_index = 0;
    
    // Build frame
    frame[frame_index++] = START_BYTE;
    frame[frame_index++] = slave_id;
    frame[frame_index++] = command;
    frame[frame_index++] = data_len;
    
    // Copy data
    if (data != NULL && data_len > 0) {
        memcpy(&frame[frame_index], data, data_len);
        frame_index += data_len;
    }
    
    // Calculate and add checksum
    uint8_t checksum = calculate_checksum(&frame[1], frame_index - 1);
    frame[frame_index++] = checksum;
    frame[frame_index++] = END_BYTE;
    
    // Send frame
    const int txBytes = uart_write_bytes(UART_NUM_2, (const char*)frame, frame_index);
    return txBytes;
}

static frame_parser_t g_frame_parser = {0};

// 状态机方式解析帧
static bool parse_frame_byte(uint8_t byte) {
    switch(g_frame_parser.state) {
        case FRAME_STATE_IDLE:
            if (byte == START_BYTE) {
                g_frame_parser.buffer[0] = byte;
                g_frame_parser.length = 1;
                g_frame_parser.state = FRAME_STATE_HEADER;
            }
            break;
            
        case FRAME_STATE_HEADER:
            if (g_frame_parser.length < 4) {
                g_frame_parser.buffer[g_frame_parser.length++] = byte;
                if (g_frame_parser.length == 4) {
                    g_frame_parser.current_data_len = g_frame_parser.buffer[3];
                    g_frame_parser.state = FRAME_STATE_DATA;
                }
            }
            break;
            
        case FRAME_STATE_DATA:
            g_frame_parser.buffer[g_frame_parser.length++] = byte;
            if (g_frame_parser.length >= 4 + g_frame_parser.current_data_len) {
                g_frame_parser.state = FRAME_STATE_CHECKSUM;
            }
            break;
            
        case FRAME_STATE_CHECKSUM:
            g_frame_parser.buffer[g_frame_parser.length++] = byte;
            g_frame_parser.state = FRAME_STATE_END;
            break;
            
        case FRAME_STATE_END:
            if (byte == END_BYTE) {
                g_frame_parser.buffer[g_frame_parser.length++] = byte;
                uint8_t calc_csum = 0;
                for (int i = 1; i < g_frame_parser.length - 2; i++) {
                    calc_csum ^= g_frame_parser.buffer[i];
                }
                if (calc_csum == g_frame_parser.buffer[g_frame_parser.length - 2]) {
                    return true; // 不清空，由调用者处理
                }
            }break;
            // fall through to error
        default:
            memset(&g_frame_parser, 0, sizeof(g_frame_parser));
            g_frame_parser.state = FRAME_STATE_IDLE;
            break;
    }

    if (g_frame_parser.length >= sizeof(g_frame_parser.buffer)) {
        g_frame_parser.state = FRAME_STATE_IDLE;
    }
    return false;
}
// 创建消息队列和事件组
static QueueHandle_t g_frame_queue = NULL;
static EventGroupHandle_t g_response_event = NULL;

// 接收任务，解析帧并发送到队列中
static void process_received_frame(const uint8_t *buffer, uint16_t length) {
    frame_message_t msg = {0};
    
    if (length < 6) return;
    
    msg.slave_id = buffer[1];
    msg.command = buffer[2];
    msg.data_len = buffer[3];
    
    if (msg.data_len > 0 && msg.data_len <= sizeof(msg.data)) {
        memcpy(msg.data, &buffer[4], msg.data_len);
    }
    

    // 发送到主通信队列（在任务上下文中）
    xQueueSend(g_frame_queue, &msg, 0);
    
    // 如果是查询响应，设置相应的事件标志（在任务上下文中）
    if (msg.command == CMD_QUERY_RESPONSE_DATA) {
        EventBits_t bits = 0;
        
        if (msg.slave_id == SLAVE_1_ID) {
            bits = RESPONSE_SLAVE_1_BIT;
        } else if (msg.slave_id == SLAVE_2_ID) {
            bits = RESPONSE_SLAVE_2_BIT;
        }
        
        if (bits != 0) {
            xEventGroupSetBits(g_response_event, bits);
        }
    }

}
// UART接收任务
static void rx_task(void *arg) {
    static const char *RX_TASK_TAG = "RX_TASK";
    uint8_t rx_buffer[128]; // 128字节缓冲区
    int len;
    
    while (1) {
        // 一次读取多个字节(最多128字节)，20ms超时。uart_read_bytes()函数内部实现环形缓冲区，数据不会丢失。
        len = uart_read_bytes(UART_NUM_2, rx_buffer, sizeof(rx_buffer),
                            pdMS_TO_TICKS(20));
        
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                if (parse_frame_byte(rx_buffer[i])) {
                    uint16_t saved_length = g_frame_parser.length;
                    uint8_t saved_buffer[sizeof(g_frame_parser.buffer)];
                    memcpy(saved_buffer, g_frame_parser.buffer, saved_length);

                    process_received_frame(saved_buffer, saved_length);
                    memset(&g_frame_parser, 0, sizeof(g_frame_parser));

                    ESP_LOGD(RX_TASK_TAG, "Received frame: %d bytes", saved_length);
                    if (esp_log_level_get(RX_TASK_TAG) >= ESP_LOG_DEBUG) {
                        ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, saved_buffer, 
                                            saved_length, ESP_LOG_DEBUG);
                    }
                }
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS); // 减少延迟时间
    }
}
// 解析从机数据任务（处理所有帧）
static void sensor_data_task(void *arg) {
    static const char *TAG = "SENSOR_DATA";
    frame_message_t msg;
    
    ESP_LOGI(TAG, "Sensor data task started");
    
    while (1) {
        // 等待所有帧数据
        if (xQueueReceive(g_frame_queue, &msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Processing data from slave %d, cmd: 0x%02X", 
                     msg.slave_id, msg.command);
            
            // 根据命令类型处理不同数据
            switch (msg.command) {
                case CMD_QUERY_RESPONSE_DATA:  // 查询响应
                    if (msg.data_len >= 8) {  // 示例：假设传感器数据为8字节
                        int16_t temperature = (msg.data[0] << 8) | msg.data[1];
                        int16_t humidity = (msg.data[2] << 8) | msg.data[3];
                        uint16_t adc_value = (msg.data[4] << 8) | msg.data[5];
                        uint8_t status = msg.data[6];
                        
                        ESP_LOGI(TAG, "Slave %d: Temp=%.1fC, Humi=%.1f%%, ADC=%d, Status=0x%02X",
                                 msg.slave_id,
                                 temperature / 10.0f,
                                 humidity / 10.0f,
                                 adc_value,
                                 status);
                        
                        // 这里可以进一步处理数据，如存储、发送到云端等
                    }
                    break;
                    
                default:
                    ESP_LOGW(TAG, "Unhandled command 0x%02X from slave %d", 
                             msg.command, msg.slave_id);
                    break;
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
// 主任务，发送查询命令并通过事件组等待响应
static void master_task(void *arg) {
    static const char *TAG = "MASTER";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    
    uint8_t current_slave = SLAVE_1_ID;
   
    const uint8_t max_retries = 3;
    
    while (1) {
        
        
        for (uint8_t retry = 0; retry < max_retries; retry++) {
            ESP_LOGI(TAG, "Querying slave %d (retry %d)", 
                     current_slave, retry + 1);
            
            send_protocol_frame(current_slave, CMD_QUERY, 
                               NULL, 0);
            
            // 等待响应事件（使用自动清除功能，避免竞态条件）
            EventBits_t expected_bits = 0;
            if (current_slave == SLAVE_1_ID) {
                expected_bits = RESPONSE_SLAVE_1_BIT;
            } else {
                expected_bits = RESPONSE_SLAVE_2_BIT;
            }
            
            EventBits_t received_bits = xEventGroupWaitBits(
                g_response_event,
                expected_bits,
                pdTRUE,  // 自动清除等待的位
                pdTRUE,  // 等待所有位
                pdMS_TO_TICKS(2000)
            );
            
            if ((received_bits & expected_bits) == expected_bits) {
                ESP_LOGI(TAG, "Received response from slave %d", current_slave);
                break; // 收到响应，退出重试循环
            }
        
            if (retry == max_retries - 1) {
                ESP_LOGW(TAG, "No response from slave %d after %d retries", 
                         current_slave, max_retries);
            }
        }
        
        // 切换到下一个从机
        current_slave = (current_slave == SLAVE_1_ID) ? SLAVE_2_ID : SLAVE_1_ID;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
// 初始化函数，创建队列和任务

void app_lora_net_init(void) {
   
    
    // 创建队列和事件组
    g_frame_queue = xQueueCreate(10, sizeof(frame_message_t));
    g_response_event = xEventGroupCreate();
    
    if (g_frame_queue == NULL || g_response_event == NULL) {
        ESP_LOGE("INIT", "Failed to create queues or event group");
        return;
    }
    
    // 创建任务
        xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
        xTaskCreate(master_task, "master_task", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
        xTaskCreate(sensor_data_task, "sensor_data_task", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
}