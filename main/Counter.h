#pragma once

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio_etm.h"
#include "esp_timer.h"


struct MeasurementData {
    int64_t timerCount = 0;
    int counterValue = 0;
    int sequenceNumber = 0;
};

class Counter {
public:
    Counter();
    void process_count_queue(void);

private:
    enum e_State {
        IDLE,
        LOW,
        HIGH, 
        LOW_COUNT,
        COLLECT
    } state;

    QueueHandle_t measurement_queue;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t pcnt_chan_a;
    int lc;
    int sequenceNumber;
    int loops;

    static void IRAM_ATTR onePpsEdgeHandler(void* arg);
};
