#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio_etm.h"
#include "nvs_flash.h"

#include "Counter.h"

#define PCNT_INPUT_SIG_IO GPIO_NUM_2
#define PCNT_INPUT_SIG_TRIGGER GPIO_NUM_6

#define PCNT_HIGH_LIMIT 32000
#define PCNT_LOW_LIMIT -1

static const char* TAG = "Counter";

Counter::Counter(CircularBuffer& dbuf) : dbuf(dbuf) {
    pcnt_unit_config_t unit_config = {};
    unit_config.high_limit = PCNT_HIGH_LIMIT;
    unit_config.low_limit = PCNT_LOW_LIMIT;
    //unit_config.flags.accum_count = true;

    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &this->pcnt_unit));

    pcnt_event_callbacks_t cbs = {};
    cbs.on_reach = overflowHandler;
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(this->pcnt_unit, &cbs, this));

    pcnt_chan_config_t chan_a_config = {};
    chan_a_config.edge_gpio_num = PCNT_INPUT_SIG_IO;
    chan_a_config.level_gpio_num = PCNT_INPUT_SIG_TRIGGER;

    ESP_ERROR_CHECK(pcnt_new_channel(this->pcnt_unit, &chan_a_config, &this->pcnt_chan_a));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(this->pcnt_unit, PCNT_HIGH_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_enable(this->pcnt_unit));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(this->pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(this->pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(this->pcnt_unit));


    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PCNT_INPUT_SIG_TRIGGER);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PCNT_INPUT_SIG_TRIGGER, onePpsEdgeHandler, this));

    this->measurement_queue = xQueueCreate(10, sizeof(MeasurementData));
    assert(this->measurement_queue != NULL);
}

void Counter::process_count_queue() {
    MeasurementData measurement;
    if (xQueueReceive(this->measurement_queue, &measurement, 0) == pdPASS) {
		// Explicitly cast PCNT_HIGH_LIMIT and measurement.overflows to uint64_t before multiplication
		uint64_t overflowsCalculated = static_cast<uint64_t>(PCNT_HIGH_LIMIT) * static_cast<uint64_t>(measurement.overflows);
		measurement.counterValue = static_cast<uint64_t>(measurement.rawCounterValue) + overflowsCalculated;
		dbuf.putFront(measurement);
        ESP_LOGI(TAG, "last %llu serial %d period %d overflows %d", measurement.counterValue, measurement.sequenceNumber, measurement.period, measurement.overflows);
    }
}

bool IRAM_ATTR Counter::overflowHandler(pcnt_unit_t* unit, const pcnt_watch_event_data_t* event, void*arg) {
    auto* inst = static_cast<Counter*>(arg);
	inst->overflows++;
	return true;
}

void IRAM_ATTR Counter::onePpsEdgeHandler(void* arg) {
    auto* inst = static_cast<Counter*>(arg);
    int level = gpio_get_level(PCNT_INPUT_SIG_TRIGGER);
    if (level == 0) {
        if (inst->state == IDLE) {
            pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
            inst->state = LOW;
        } else if (inst->state == HIGH) {
            if (++inst->lc < inst->loops) {
                pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
                inst->state = LOW;
            } else {
                pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_HOLD, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
                inst->state = LOW_COUNT;
            }
        } else if (inst->state == COLLECT) {
            pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
            MeasurementData vv;
            vv.timerCount = esp_timer_get_time();
            vv.sequenceNumber = inst->sequenceNumber++;
			vv.period = inst->lc; // inst->loops;
			vv.overflows = inst->overflows;
			inst->lc = 0;
			inst->overflows = 0;
            pcnt_unit_get_count(inst->pcnt_unit, &vv.rawCounterValue);
            pcnt_unit_clear_count(inst->pcnt_unit);
            xQueueSendFromISR(inst->measurement_queue, &vv, NULL);
            inst->state = LOW;
        }
    } else {
        if (inst->state == LOW) {
            pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
            inst->state = HIGH;
        } else if (inst->state == LOW_COUNT) {
            pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_HOLD, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
            inst->state = COLLECT;
        }
    }
}
