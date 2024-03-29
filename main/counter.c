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
#include "nvs_flash.h"


// static const char *TAG = "counter";

#define PCNT_INPUT_SIG_IO   17 // Pulse Input GPIO
#define PCNT_INPUT_SIG_TRIGGER   GPIO_NUM_15 // Pulse Input GPIO

#define PCNT_HIGH_LIMIT 20000
#define PCNT_LOW_LIMIT  -1

QueueHandle_t count_queue;

pcnt_unit_handle_t pcnt_unit = NULL;
pcnt_channel_handle_t pcnt_chan_a = NULL;

enum e_State {
	IDLE,
	LOW,
	HIGH, 
	LOW_COUNT,
	COLLECT,
} state = IDLE;

const int loops = 10;
int lc = 0;

static void IRAM_ATTR one_pps_edge_handler(void* arg) {
    int level = gpio_get_level(PCNT_INPUT_SIG_TRIGGER);
    if (level == 0) {	// now low
       	if (state == IDLE) {
			// hit low, when high KEEP (count), when low HOLD (don't count until high)
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
			state = LOW;
		} else if (state == HIGH) {
			// in low, state was high
			// when high, hold, keep counting while low
			if (++lc < loops) {
				pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
				state = LOW;
			} else {
				lc = 0;
				pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_HOLD, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
				state = LOW_COUNT;
			}
		} else if (state == COLLECT) {
			// ws high, now collecting value, next high start counting, hold while low
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
			int count_value;
            pcnt_unit_get_count(pcnt_unit, &count_value);
            pcnt_unit_clear_count(pcnt_unit);
            xQueueSendFromISR(count_queue, &count_value, NULL);
			state = LOW;
		}
    } else { // now high
		// got high in low state
       if (state == LOW) {
		   // in high, keep  counting, when low, keep counting
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
			state = HIGH;
		} else if (state == LOW_COUNT) { // high after low count
			// stop counting
			// from LOW, high state will be hold, but low state will be KEEP, and counting so hold all
			pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_HOLD, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
			state = COLLECT;
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
		.level_gpio_num = PCNT_INPUT_SIG_TRIGGER
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
	// when counter is reached call internally set up ISR
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, PCNT_HIGH_LIMIT));
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

	ESP_ERROR_CHECK(gpio_install_isr_service(0));
	gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_ANYEDGE,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = (1ULL << PCNT_INPUT_SIG_TRIGGER),
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE
	};
	ESP_ERROR_CHECK(gpio_config(&io_conf));
	ESP_ERROR_CHECK(gpio_isr_handler_add(PCNT_INPUT_SIG_TRIGGER, one_pps_edge_handler, NULL));

	count_queue = xQueueCreate(10, sizeof(int));
	assert(count_queue != NULL);
}

void display_count(void) {
	int count_value;
	if (xQueueReceive(count_queue, &count_value, pdMS_TO_TICKS(300)) == pdPASS) {
		printf("counter %d\n", count_value); 
	}
}
