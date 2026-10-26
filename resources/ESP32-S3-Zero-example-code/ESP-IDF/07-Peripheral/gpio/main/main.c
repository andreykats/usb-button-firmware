#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "example";

static const gpio_num_t GPIO_OUTPUT_LED = GPIO_NUM_7;

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_LED),  // Select GPIOs to configure
        .mode = GPIO_MODE_OUTPUT,              // Set as output mode
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE         // Disable interrupt
    };
    gpio_config(&io_conf);

    while (1) {
        // Set GPIO high
        ESP_LOGI(TAG, "Turn the LED on");
        gpio_set_level(GPIO_OUTPUT_LED, 1);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second

        // Set GPIO low
        ESP_LOGI(TAG, "Turn the LED off");
        gpio_set_level(GPIO_OUTPUT_LED, 0);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}