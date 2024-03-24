#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"  // Use the appropriate PCNT driver header
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"


static const char *TAG = "counter";

#define PCNT_INPUT_SIG_IO   20 // Pulse Input GPIO
#define PCNT_INPUT_SIG_TRIGGER   GPIO_NUM_22 // Pulse Input GPIO

#define PCNT_HIGH_LIMIT 20000
#define PCNT_LOW_LIMIT  -1

static volatile int cnt = 0;
static volatile int trigger_cnt = 0;
static volatile int overflow_cnt = 0;

QueueHandle_t queue;
pcnt_unit_handle_t pcnt_unit = NULL;
pcnt_channel_handle_t pcnt_chan_a = NULL;


/*static void IRAM_ATTR one_pps_edge_handler(void* arg) {
    trigger_cnt++;
} */

// called when the counter overlows
static bool IRAM_ATTR overflow_handler(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    overflow_cnt++;
    return false;
}

void counter_init(void) {
    pcnt_unit_config_t unit_config = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT,
//		.flags.accum_count = true
//		.flags.io_loop_back = true
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = PCNT_INPUT_SIG_IO,
		.level_gpio_num = -1
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
	// when counter is reached call ISR
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, PCNT_HIGH_LIMIT));
    pcnt_event_callbacks_t cbs = {
        .on_reach = overflow_handler,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);
    pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}


void app_main() {
    queue = xQueueCreate(10, sizeof(int));

	extern void init_ledc_square_wave();
	extern void start_pulse_timer();

    init_ledc_square_wave();
    start_pulse_timer();
	counter_init();


    while (1) {
        vTaskDelay(pdMS_TO_TICKS(500)); 
		printf("triggers %d overflows %d\n", trigger_cnt, overflow_cnt);
        int count_value;
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &count_value));
        printf("Current counter value: %d\n", count_value); 
    }
}
