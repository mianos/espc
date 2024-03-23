#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio.h"


static const char *TAG = "testtimer";

#define PULSE_PIN GPIO_NUM_21 // Change this to the desired GPIO pin number

esp_timer_handle_t timer_handle;

void IRAM_ATTR timer_callback(void *arg) {
    static bool state = false;
    gpio_set_level(PULSE_PIN, state);
    state = !state;
}

void start_pulse_timer(void) {
    gpio_reset_pin(PULSE_PIN);
    gpio_set_direction(PULSE_PIN, GPIO_MODE_OUTPUT);

    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "pulse_timer"
    };

    esp_timer_create(&timer_args, &timer_handle);
    esp_timer_start_periodic(timer_handle, 500000); // .5 second square wave
}
