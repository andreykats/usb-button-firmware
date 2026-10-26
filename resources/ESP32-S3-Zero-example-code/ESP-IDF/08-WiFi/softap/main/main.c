#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "wifi example";

// --- AP (Access Point) Configuration ---
#define ESP_WIFI_SSID "esp32_s3_test"
#define ESP_WIFI_PASS "12345678"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN 2

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    // Simple event handler, just prints the event ID.
    // In a real application, you would handle events like STA connection/disconnection here.
    printf("Event nr: %ld!\n", event_id);
}

void wifi_init_softap()
{
    // 1. Initialization phase
    // Initialize the underlying TCP/IP stack
    esp_netif_init();
    // Create the default event loop
    esp_event_loop_create_default();
    // Create the default Wi-Fi AP network interface
    esp_netif_create_default_wifi_ap();

    // Get the default Wi-Fi initialization config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // Initialize the Wi-Fi driver with the default config
    esp_wifi_init(&cfg);
    // Register the Wi-Fi event handler to listen for all Wi-Fi events
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    // 2. Configuration phase
    // Define the Wi-Fi configuration structure
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
        },
    };


    // 3. Start phase
    // Set the Wi-Fi mode to AP
    esp_wifi_set_mode(WIFI_MODE_AP);
    // Apply the configuration to the Wi-Fi AP interface
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    // Start Wi-Fi
    esp_wifi_start();

    // Log that AP is started and show its SSID, password, and channel
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
}

void app_main(void)
{
    // Initialize and start the Wi-Fi AP
    wifi_init_softap();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}