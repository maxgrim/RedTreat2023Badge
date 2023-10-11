#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <http_app.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <wifi_manager.h>

#include "config.h"
#include "filesystem.h"
#include "leds.h"
#include "mqtt.h"
#include "sound.h"
#include "uuid.h"

#define GPIO_INPUT_IO_BOOT 0

static const char *TAG = "redtreat_main";

static char uuid[UUID_STR_LEN + 1];
static bool wifi_connected_before = false;

static TaskHandle_t gpioMonitorTaskHandle = NULL;
static int bootButtonPressedCount = 0;

static QueueHandle_t mainQueueHandle = NULL;
static TaskHandle_t mainQueueTaskHandle = NULL;

typedef struct main_queue_message {
    int type;
    char sound_path[128];
    int color;
} main_queue_message;

void blink_leds_from_task(int *color) {
    leds_blink(*color);
    vTaskDelete(NULL);
}

static void message_task() {
    main_queue_message queue_message;

    while (1) {
        if (xQueueReceive(mainQueueHandle, &(queue_message), (TickType_t)5)) {
            if (queue_message.type == 1) {
                xTaskCreate((void *)blink_leds_from_task, "blink_leds_from_task", 4096, &queue_message.color, 10, NULL);
            }

            sound_play(queue_message.sound_path);
        }
    }
}

void mqtt_message_callback(char *message, int len) {
    if (len > 127) {
        len = 127;
    }

    message[len] = '\0';

    char *token = strtok(message, ",");
    char parsed_message[2][128];

    for (int i = 0; i < 2; i++) {
        if (token == NULL) {
            ESP_LOGE(TAG, "Received an inavlid message");
            return;
        }

        snprintf(parsed_message[i], len, "%s", token);
        token = strtok(NULL, ",");
    }

    ESP_LOGI(TAG, "sound_file=%s", parsed_message[0]);
    ESP_LOGI(TAG, "led_effect=%s", parsed_message[1]);

    char *endptr;
    int parsed_number = (int)strtol(parsed_message[1], &endptr, 16);

    if (endptr == parsed_message[1]) {
        ESP_LOGW(TAG, "Received an invalid LED color");
        parsed_number = 0x000000;
    } else if (*endptr != '\0') {
        ESP_LOGW(TAG, "Received an invalid LED color");
        parsed_number = 0x000000;
    }

    main_queue_message queue_message = {
        .type = 1,
        .color = parsed_number};

    strncpy(queue_message.sound_path, parsed_message[0], 128);

    xQueueSend(mainQueueHandle, (void *)&queue_message, (TickType_t)0);
}

void mqtt_connect_callback() {
    main_queue_message queue_message = {
        .type = 0};
    strncpy(queue_message.sound_path, "/storage/_mqtt_connected.wav", 128);
    xQueueSend(mainQueueHandle, (void *)&queue_message, (TickType_t)0);

    leds_fill(0);
}

void mqtt_disconnect_callback() {
    main_queue_message queue_message = {
        .type = 0};
    strncpy(queue_message.sound_path, "/storage/_mqtt_disconnected.wav", 128);
    xQueueSend(mainQueueHandle, (void *)&queue_message, (TickType_t)0);
}

void mqtt_error_callback() {
    main_queue_message queue_message = {
        .type = 0};
    strncpy(queue_message.sound_path, "/storage/_mqtt_failed.wav", 128);
    xQueueSend(mainQueueHandle, (void *)&queue_message, (TickType_t)0);
}

static void wifi_connected(void *pvParameter) {
    ip_event_got_ip_t *param = (ip_event_got_ip_t *)pvParameter;
    char str_ip[16];

    // Transform the IP address to human readable string
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
    ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

    wifi_connected_before = true;

    main_queue_message queue_message = {
        .type = 0};
    strncpy(queue_message.sound_path, "/storage/_wifi_connected.wav", 128);
    xQueueSend(mainQueueHandle, (void *)&queue_message, (TickType_t)0);

    // Initiate and start MQTT
    mqtt_app_start(uuid, &mqtt_message_callback, &mqtt_connect_callback, &mqtt_disconnect_callback, &mqtt_error_callback);
}

static esp_err_t http_get_handler(httpd_req_t *req) {
    if (strcmp(req->uri, "/uuid") == 0) {
        ESP_LOGI(TAG, "Serving page /uuid");

        char response[80];
        snprintf(response, 80, "<html><body><p>%s</p></body></html>", uuid);

        httpd_resp_set_status(req, "200 OK");
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, response, strlen(response));
    } else {
        httpd_resp_send_404(req);
    }

    return ESP_OK;
}

void wifi_disconnected(void *pvParameter) {
    if (wifi_connected_before) {
        main_queue_message queue_message = {
            .type = 0};
        strncpy(queue_message.sound_path, "/storage/_wifi_disconnected.wav", 128);
        xQueueSend(mainQueueHandle, (void *)&queue_message, (TickType_t)0);
    }
}

void wifi_ap(void *pvParameter) {
    main_queue_message queue_message = {
        .type = 0};
    strncpy(queue_message.sound_path, "/storage/_wifi_ap.wav", 128);
    xQueueSend(mainQueueHandle, (void *)&queue_message, (TickType_t)0);
}

void gpio_monitor_task() {
    while (1) {
        if (gpio_get_level(GPIO_INPUT_IO_BOOT) == 0) {
            bootButtonPressedCount++;
        } else {
            bootButtonPressedCount = 0;
        }

        if (bootButtonPressedCount >= 5) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            delete_uuid();
            esp_restart();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Add 1 tick delay (10 ms) so that current task does not starve idle task and trigger watchdog timer
    }
}

void app_main(void) {
    // Initiating NVS flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    main_queue_message queue_message;
    mainQueueHandle = xQueueCreate(6, sizeof(queue_message));

    if (mainQueueHandle == 0) {
        ESP_LOGE(TAG, "Failed to create mainQueueHandle= %p\n", mainQueueHandle);
    }

    xTaskCreate(message_task, "message_task", 4096, NULL, 4, &mainQueueTaskHandle);

    // Clear the LEDs
    leds_init();
    leds_fill(0);

    // Initiating the filesystem
    ESP_ERROR_CHECK(fs_init());

    // Initiate sound
    ESP_ERROR_CHECK(sound_init());

    // Generating unique ID
    get_uuid(uuid);

    // Start the WiFi manager
    wifi_manager_start();
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &wifi_connected);
    wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &wifi_disconnected);
    wifi_manager_set_callback(WM_ORDER_START_AP, &wifi_ap);
    http_app_set_handler_hook(HTTP_GET, &http_get_handler);

    gpio_set_direction(GPIO_INPUT_IO_BOOT, GPIO_MODE_INPUT);

    // Task to monitor for resetting WiFi settings
    xTaskCreate(gpio_monitor_task, "gpio_monitor_task", 4096, NULL, 10, &gpioMonitorTaskHandle);
}
