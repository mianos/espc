#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"  // Use the appropriate PCNT driver header
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio_etm.h"


// static const char *TAG = "counter";

#define PCNT_INPUT_SIG_IO   20 // Pulse Input GPIO
#define PCNT_INPUT_SIG_TRIGGER   GPIO_NUM_22 // Pulse Input GPIO

#define PCNT_HIGH_LIMIT 20000
#define PCNT_LOW_LIMIT  -1

static volatile int trigger_cnt = 0;
static volatile int overflow_cnt = 0;
int count_value;

pcnt_unit_handle_t pcnt_unit = NULL;
pcnt_channel_handle_t pcnt_chan_a = NULL;

enum e_State {
	IDLE,
	LOW,
	HIGH, 
	LOW_COUNT
} state = IDLE;

static void IRAM_ATTR one_pps_edge_handler(void* arg) {
    trigger_cnt++;
    int level = gpio_get_level(PCNT_INPUT_SIG_TRIGGER);
    if (level == 0) {	// now low
       	if (state == IDLE) {
			// hit low, when high KEEP (count), when low HOLD (don't count until high)
			pcnt_unit_get_count(pcnt_unit, &count_value);
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
			state = LOW;
		} else if (state == HIGH) {
			// in low, state was high
			// when high, hold, keep counting while low
			pcnt_unit_get_count(pcnt_unit, &count_value);
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_HOLD, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
			state = LOW_COUNT;
		}
    } else { // now high
		// got high in low state
       if (state == LOW) {
		   // in high, keep  counting, when low, keep counting
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
			pcnt_unit_get_count(pcnt_unit, &count_value);
			state = HIGH;
		} else if (state == LOW_COUNT) { // high after low count
			// stop counting
			// from LOW, high state will be hold, but low state will be KEEP, and counting so hold all
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_HOLD, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
			pcnt_unit_get_count(pcnt_unit, &count_value);
			pcnt_unit_clear_count(pcnt_unit);
			state = IDLE;
		}
    }
}


void counter_init(void) {
    pcnt_unit_config_t unit_config = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT,
		.flags.accum_count = true
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = PCNT_INPUT_SIG_IO,
		.level_gpio_num = GPIO_NUM_22
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
	// when counter is reached call internally set up ISR
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, PCNT_HIGH_LIMIT));
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

	gpio_install_isr_service(0);
	gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_ANYEDGE,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = (1ULL << PCNT_INPUT_SIG_TRIGGER),
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE
	};
	gpio_config(&io_conf);
	gpio_isr_handler_add(PCNT_INPUT_SIG_TRIGGER, one_pps_edge_handler, NULL);
}


void app_main() {

	extern void init_ledc_square_wave();
	extern void start_pulse_timer();

    init_ledc_square_wave();
    start_pulse_timer();
	counter_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(300)); 
		printf("triggers %d overflows %d counter %d state %d\n", trigger_cnt, overflow_cnt, count_value, state); 
    }
}
