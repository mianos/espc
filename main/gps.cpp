#include "gps.h"
#include "esp_log.h"

static const char* TAG = "GPSReader";

GPSReader::GPSReader(uart_port_t uartPort, int txPin, int rxPin, int baudRate)
    : uartPort(uartPort), txPin(txPin), rxPin(rxPin), baudRate(baudRate) {}

GPSReader::~GPSReader() {
    uart_driver_delete(uartPort);
}

void GPSReader::initialize() {
    uart_config_t uartConfig = {};
    uartConfig.baud_rate = baudRate;
    uartConfig.data_bits = UART_DATA_8_BITS;
    uartConfig.parity = UART_PARITY_DISABLE;
    uartConfig.stop_bits = UART_STOP_BITS_1;
    uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uartConfig.rx_flow_ctrl_thresh = 122;
    uartConfig.source_clk = UART_SCLK_APB;

    uart_param_config(uartPort, &uartConfig);
    uart_set_pin(uartPort, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(uartPort, bufferSize * 2, 0, 0, nullptr, 0);
}

void GPSReader::startReadLoopTask() {
    xTaskCreate(readLoopTask, "gps_read_loop", 4096, this, 5, NULL);
}

void GPSReader::readLoopTask(void* pvParameters) {
    GPSReader* reader = static_cast<GPSReader*>(pvParameters);
    reader->readLoop();
}

void GPSReader::readLoop() {
    uint8_t* data = new uint8_t[bufferSize];
    while (true) {
        // Read data from UART port
        int len = uart_read_bytes(uartPort, data, bufferSize - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the received data to ensure it's a valid C-style string
            
            // Convert the read bytes to a std::string
            std::string receivedData(reinterpret_cast<char*>(data), len);
            
            // Pass the received NMEA sentence to the decoder
            // The decoder will parse the sentence into fields and process it as configured
            decoder.parseData(receivedData);
        }
    }
    delete[] data; // Cleanup, though this line is technically unreachable
}
