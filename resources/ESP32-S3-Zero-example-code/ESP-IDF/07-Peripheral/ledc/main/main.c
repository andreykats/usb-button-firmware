#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "example"; // Define log tag

static const gpio_num_t LED_PIN = GPIO_NUM_7; // GPIO pin for the LED
#define LEDC_CHANNEL LEDC_CHANNEL_0           // LEDC channel
#define LEDC_TIMER LEDC_TIMER_0               // LEDC timer
#define LEDC_MODE LEDC_LOW_SPEED_MODE         // LEDC speed mode, low-speed mode
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT       // PWM duty cycle resolution (10-bit, 0-1023)
#define LEDC_FREQUENCY 1000                   // PWM frequency (1 kHz)

void app_main(void)
{
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,          // Set speed mode
        .duty_resolution = LEDC_DUTY_RES, // Set duty cycle resolution
        .timer_num = LEDC_TIMER,          // Select timer
        .freq_hz = LEDC_FREQUENCY         // Set PWM frequency
    };
    ledc_timer_config(&ledc_timer);       // Apply timer configuration

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num = LED_PIN,     // GPIO pin for the channel's output
        .speed_mode = LEDC_MODE, // Set speed mode (must be same as timer)
        .channel = LEDC_CHANNEL, // Select LEDC channel
        .timer_sel = LEDC_TIMER, // Select timer for this channel
        .duty = 0                // Set initial duty cycle to 0
    };
    ledc_channel_config(&ledc_channel); // Apply channel configuration

    while (1)
    {
        // Fade in
        ESP_LOGI(TAG, "LED Fade in");
        for (int duty = 0; duty <= 1023; duty += 16)
        {
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty); // Set duty cycle
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);    // Update duty cycle to take effect
            vTaskDelay(pdMS_TO_TICKS(20));                // Delay for 20 ms
        }

        // Fade out
        ESP_LOGI(TAG, "LED Fade out");
        for (int duty = 1023; duty >= 0; duty -= 16)
        {
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty); // Set duty cycle
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);    // Update duty cycle to take effect
            vTaskDelay(pdMS_TO_TICKS(20));                // Delay for 20 ms
        }
    }
}