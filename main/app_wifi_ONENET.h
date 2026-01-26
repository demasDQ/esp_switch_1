#ifndef _APP_WIFI_ONENET_H_
#define _APP_WIFI_ONENET_H_

/* 标准库头文件 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* ESP-IDF 核心头文件 */
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

/* FreeRTOS 头文件 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

/* LwIP 网络栈头文件 */
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

/* 项目组件头文件 */
#include "wifi_manager.h"
#include "mqtt_client.h"

/* JSON 处理 */
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 函数声明可以放在这里 */

void app_wifi_ONENET_init(void);


#ifdef __cplusplus
}
#endif
#endif /* _APP_WIFI_ONENET_H_ */