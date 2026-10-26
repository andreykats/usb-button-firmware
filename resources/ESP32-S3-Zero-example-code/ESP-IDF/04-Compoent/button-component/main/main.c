#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "iot_button.h"
#include "button_gpio.h"

#define BUTTON_GPIO GPIO_NUM_7
#define BUTTON_ACTIVE_LEVEL 0

static const char *TAG = "button_example";

// Single click callback function
static void button_single_click_cb(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");
}

// Double click callback function
static void button_double_click_cb(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "BUTTON_DOUBLE_CLICK");
}

void app_main(void)
{
    // Define button configuration
    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = BUTTON_GPIO,             // GPIO number connected to the button
        .active_level = BUTTON_ACTIVE_LEVEL, // Active level (0 for low active, 1 for high active)
    };

    // Create button handle
    button_handle_t gpio_btn = NULL;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &gpio_btn);
    if (gpio_btn == NULL)
    {
        ESP_LOGE(TAG, "Button create failed");
        return;
    }

    // Register single click event
    iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, NULL, button_single_click_cb, NULL);

    // Register double click event
    iot_button_register_cb(gpio_btn, BUTTON_DOUBLE_CLICK, NULL, button_double_click_cb, NULL);

    // Main loop
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}