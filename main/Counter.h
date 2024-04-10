#pragma once
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio_etm.h"
#include "esp_timer.h"
#include "Measurement.h"
#include "CircularBuffer.h"



class Counter {
public:
    Counter(CircularBuffer& dbuf);
    void process_count_queue(void);

	void set_loops(int loops_) {
		loops = loops_;
	}
private:
	CircularBuffer& dbuf;
    enum e_State {
        IDLE,
        LOW,
        HIGH, 
        LOW_COUNT,
        COLLECT
    } state = IDLE;

    QueueHandle_t measurement_queue;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t pcnt_chan_a;
    int lc = 0;
    int sequenceNumber = 0;
    int loops = 10;
    int overflows = 0;

    static bool IRAM_ATTR overflowHandler(pcnt_unit_t* unit, const pcnt_watch_event_data_t* event, void*arg);
    static void IRAM_ATTR onePpsEdgeHandler(void* arg);
};
