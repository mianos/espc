#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"  // Use the appropriate PCNT driver header
// #include "driver/pcnt.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"


static const char *TAG = "example";
#define PCNT_INPUT_SIG_IO   20 // Pulse Input GPIO

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


#define EXAMPLE_PCNT_HIGH_LIMIT 20000
#define EXAMPLE_PCNT_LOW_LIMIT  -1

static volatile int cnt = 0;

QueueHandle_t queue;
pcnt_unit_handle_t pcnt_unit = NULL;

static bool example_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
	cnt++;
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    // send event data to queue, from this interrupt callback
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}

void pcnt(void)
{
    ESP_LOGI(TAG, "install pcnt unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    ESP_LOGI(TAG, "install pcnt channel");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = PCNT_INPUT_SIG_IO,
        .level_gpio_num = -1
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    ESP_LOGI(TAG, "set edge action for pcnt channel");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));

    ESP_LOGI(TAG, "add watch point and register callback");
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, EXAMPLE_PCNT_HIGH_LIMIT));
    pcnt_event_callbacks_t cbs = {
        .on_reach = example_pcnt_on_reach,
    };
    QueueHandle_t queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));

    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}




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

void app_main() {
    queue = xQueueCreate(10, sizeof(int));

    init_ledc_square_wave();
    pcnt();
    start_pulse_timer();

    // Report counter value
    int pulse_count = 0;
    int event_count = 0;
    int pin22_state = 0;

    while (1) {
        if (xQueueReceive(queue, &event_count, pdMS_TO_TICKS(1000))) {
            ESP_LOGI(TAG, "Watch point event, count: %d", event_count);
        } else {
            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
            ESP_LOGI(TAG, "Pulse count: %d", pulse_count);
        }

        // Read the state of pin 22
        pin22_state = gpio_get_level(GPIO_NUM_22);

        // Print the state of pin 22 on the console
        ESP_LOGI(TAG, "Pin 22 state: %d", pin22_state);

        vTaskDelay(pdMS_TO_TICKS(500)); 

        printf("cnt is %d\n", cnt);
    }
}
