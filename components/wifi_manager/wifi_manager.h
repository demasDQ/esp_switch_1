#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_

#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#ifdef __cplusplus
extern "C" {
#endif

// WiFi配置参数
#define WIFI_SSID      "Redmik50"
#define WIFI_PASS      "12345678"
#define MAX_RETRY      5

// WiFi初始化函数
void wifi_init(void);

// WiFi事件处理函数
void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                       int32_t event_id, void* event_data);

#ifdef __cplusplus
}
#endif

#endif /* _WIFI_MANAGER_H_ */
