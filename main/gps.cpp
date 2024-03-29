#include "gps.h"

GPSReader::GPSReader(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate)
    : uart_num(uart_num), tx_pin(tx_pin), rx_pin(rx_pin), baud_rate(baud_rate) {}

GPSReader::~GPSReader() {
    uart_driver_delete(uart_num);
}

void GPSReader::initialize() {
    uart_config_t uart_config;
    uart_config.baud_rate = baud_rate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;
    uart_config.source_clk = UART_SCLK_APB;
    uart_param_config(uart_num, &uart_config);

    uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(uart_num, buffer_size * 2, 0, 0, nullptr, 0);
}

void GPSReader::readLoop() {
    uint8_t* data = new uint8_t[buffer_size];
    while (true) {
        int len = uart_read_bytes(uart_num, data, buffer_size, pdMS_TO_TICKS(1000));
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the data
            ESP_LOGI("GPSReader", "Received: %s", data);
        }
    }
    delete[] data;
}


void GPSReader::startReadLoopTask() {
    xTaskCreate(&GPSReader::readLoopTask, "gps_read_loop", 2048, this, 5, NULL);
}

void GPSReader::readLoopTask(void *pvParameters) {
    GPSReader *reader = static_cast<GPSReader*>(pvParameters);
    reader->readLoop();
}
