#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio_etm.h"
#include "nvs_flash.h"

#include "Counter.h"

#define PCNT_INPUT_SIG_IO 15
#define PCNT_INPUT_SIG_TRIGGER GPIO_NUM_10

#define PCNT_HIGH_LIMIT 20000
#define PCNT_LOW_LIMIT -1

Counter::Counter(CircularBuffer& dbuf) : dbuf(dbuf), state(IDLE), lc(0), sequenceNumber(0), loops(10) {
    pcnt_unit_config_t unit_config = {};
    unit_config.high_limit = PCNT_HIGH_LIMIT;
    unit_config.low_limit = PCNT_LOW_LIMIT;
    unit_config.flags.accum_count = true;

    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &this->pcnt_unit));

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
		dbuf.putFront(measurement);
        ESP_LOGI("Counter", "last %d serial %d", measurement.counterValue, measurement.sequenceNumber);
    }
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
                inst->lc = 0;
                pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_HOLD, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
                inst->state = LOW_COUNT;
            }
        } else if (inst->state == COLLECT) {
            pcnt_channel_set_level_action(inst->pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
            MeasurementData vv;
            vv.timerCount = esp_timer_get_time();
            vv.sequenceNumber = inst->sequenceNumber++;
            pcnt_unit_get_count(inst->pcnt_unit, &vv.counterValue);
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
