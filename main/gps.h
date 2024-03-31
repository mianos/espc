#pragma once

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "GNSSDecoder.h" // Make sure this is included for the NMEADecoder

class GPSReader {
public:
    GPSReader(uart_port_t uartPort, int txPin, int rxPin, int baudRate);
    ~GPSReader();
    void initialize();
    void startReadLoopTask();

private:
    uart_port_t uartPort;
    int txPin;
    int rxPin;
    int baudRate;
    static constexpr size_t bufferSize = 1024; // Adjust based on expected sentence lengths
    GNSSDecoder decoder; // Instance of the NMEADecoder for parsing

    static void readLoopTask(void* pvParameters);
    void readLoop();
};

