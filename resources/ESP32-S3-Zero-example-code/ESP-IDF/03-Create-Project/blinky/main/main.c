#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const gpio_num_t led_pin = GPIO_NUM_7;

void app_main(void)
{
    gpio_reset_pin(led_pin);                       // Reset pin
    gpio_set_direction(led_pin, GPIO_MODE_OUTPUT); // Set pin as output mode

    while (1)
    {
        gpio_set_level(led_pin, 1);      // Set pin to high, turn on LED
        printf("LED state: ON\n");       // Print current LED state
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay task for 1000 ms (1 second)

        gpio_set_level(led_pin, 0);      // Set pin to low, turn off LED
        printf("LED state: OFF\n");      // Print current LED state
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay task again for 1000 ms (1 second)
    }
}