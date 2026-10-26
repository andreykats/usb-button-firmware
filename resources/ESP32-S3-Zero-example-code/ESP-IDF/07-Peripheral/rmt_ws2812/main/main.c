#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "example";

#define BLINK_GPIO 21 // GPIO pin for the LED

static led_strip_handle_t led_strip; // LED strip handle

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring addressable LED on GPIO %d", BLINK_GPIO);

    // LED strip general configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,                                // Set GPIO pin
        .max_leds = 1,                                               // Set number of LEDs
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB, // Set color format (for ESP32-S3-Zero)
    };

    // RMT backend specific configuration
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // RMT resolution: 10MHz
        .flags.with_dma = false,           // Disable DMA
    };

    // Create LED strip object
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    led_strip_clear(led_strip); // Clear the strip on initialization
}

void app_main(void)
{
    configure_led(); // Configure the LED

    while (1)
    {

        ESP_LOGI(TAG, "Set LED color to RED");
        led_strip_set_pixel(led_strip, 0, 255, 0, 0); // Set to red (index,r,g,b)
        led_strip_refresh(led_strip);                 // Refresh the strip to show the color
        vTaskDelay(pdMS_TO_TICKS(1000));              // Delay for 1000 ms

        ESP_LOGI(TAG, "Clear LED color");
        led_strip_clear(led_strip);      // Clear the strip, turning off the LED
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 ms

        ESP_LOGI(TAG, "Set LED color to GREEN");
        led_strip_set_pixel(led_strip, 0, 0, 255, 0); // Set to green (index,r,g,b)
        led_strip_refresh(led_strip);                 // Refresh the strip to show the color
        vTaskDelay(pdMS_TO_TICKS(1000));              // Delay for 1000 ms

        ESP_LOGI(TAG, "Clear LED color");
        led_strip_clear(led_strip);      // Clear the strip, turning off the LED
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 ms
    }
}