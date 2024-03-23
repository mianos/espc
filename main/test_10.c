#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"


// Define the configuration parameters for the LEDC
#define LEDC_CHANNEL           LEDC_CHANNEL_0
#define LEDC_MODE              LEDC_LOW_SPEED_MODE
#define LEDC_GPIO_NUM          19
#define LEDC_TIMER             LEDC_TIMER_0
#define LEDC_FREQUENCY         10000000 // Target frequency of 10MHz
#define LEDC_RESOLUTION        LEDC_TIMER_1_BIT // Minimum resolution for maximum frequency

void init_ledc_square_wave() {
    // Initialize timer configuration
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_RESOLUTION, // Set the timer resolution
        .freq_hz = LEDC_FREQUENCY,          // Target frequency for the LEDC
        .speed_mode = LEDC_MODE,            // Timer mode
        .timer_num = LEDC_TIMER,            // Timer index
        .clk_cfg = LEDC_AUTO_CLK,           // Automatically select the clock source
    };
    ledc_timer_config(&ledc_timer);

    // Initialize channel configuration
    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL,
        .duty = 1,                         // Set duty cycle to 50%
        .gpio_num = LEDC_GPIO_NUM,         // GPIO number for the LEDC output
        .speed_mode = LEDC_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER
    };
    ledc_channel_config(&ledc_channel);
}

