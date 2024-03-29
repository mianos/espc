#pragma once

#include "driver/uart.h"
#include "esp_log.h"

class GPSReader {
public:
    GPSReader(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate);
    ~GPSReader();
    void initialize();
    void readLoop();
	void startReadLoopTask();
private:
    uart_port_t uart_num;
    int tx_pin;
    int rx_pin;
    int baud_rate;
    static constexpr size_t buffer_size = 1024; // Adjust buffer size as needed
	static void readLoopTask(void *pvParameters);
};
