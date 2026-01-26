/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "app_wifi_ONENET.h"

static const char *TAG = "MQTT_EXAMPLE";


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "$sys/GkQ8q42xq6/device1/thing/property/post", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "$sys/GkQ8q42xq6/device1/thing/property/post/reply", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "$sys/GkQ8q42xq6/device1/thing/property/set", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        //如果主题是 $sys/GkQ8q42xq6/device1/thing/property/set，那么回复收到
        if (event->topic_len == strlen("$sys/GkQ8q42xq6/device1/thing/property/set") &&
            memcmp(event->topic, "$sys/GkQ8q42xq6/device1/thing/property/set", event->topic_len) == 0) {
            cJSON *root = cJSON_Parse(event->data);
            if (root) {
                cJSON *id = cJSON_GetObjectItem(root, "id");
                
                // 解析参数并执行控制操作
                cJSON *params = cJSON_GetObjectItem(root, "params");
                if (params) {
                    cJSON *fill_light = cJSON_GetObjectItem(params, "fill_light");
                    cJSON *fill_water = cJSON_GetObjectItem(params, "fill_water");
                    
                    if (cJSON_IsBool(fill_light)) {
                        bool light_state = fill_light->valueint;
                        ESP_LOGI(TAG, "Fill light control: %s", light_state ? "ON" : "OFF");
                        // 这里添加控制补光灯的实际代码
                    }
                    
                    if (cJSON_IsBool(fill_water)) {
                        bool water_state = fill_water->valueint;
                        ESP_LOGI(TAG, "Fill water control: %s", water_state ? "ON" : "OFF");
                        // 这里添加控制补水器的实际代码
                    }
                }
                
                // 回复格式: {"id":"<id>","code":200,"msg":"success"}，id要根据收到id来设置，通过json解析收到的id
                if (cJSON_IsString(id) && (id->valuestring != NULL)) {
                    char reply_data[128];
                    snprintf(reply_data, sizeof(reply_data), "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}", id->valuestring);
                    msg_id = esp_mqtt_client_publish(client, "$sys/GkQ8q42xq6/device1/thing/property/set_reply", reply_data, 0, 1, 0);
                    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                }
                cJSON_Delete(root);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON data");
            }
        }
        break;


        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
          .broker = {
            .address = {
                .uri = "mqtt://mqtts.heclouds.com:1883",  // OneNET 的公开 MQTT 接入点
            }
        },
        
        // 2. 配置身份凭证
        .credentials = {
            .username = "GkQ8q42xq6",     // OneNET 平台的产品 ID
            .client_id = "device1",      // OneNET 平台中注册的设备名称
            
            // 3. 配置安全令牌（Token），采用 OneNET 平台的认证算法生成
            .authentication = {
                .password = "version=2018-10-31&res=products%2FGkQ8q42xq6%2Fdevices%2Fdevice1&et=1895584077&method=md5&sign=2FlVi0sbYizu7XbrsfW7hg%3D%3D"  // 此令牌包含签名信息，用于鉴权
            }
        }
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_wifi_ONENET_init(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);


    mqtt_app_start();
}
