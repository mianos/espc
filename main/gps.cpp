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
        int len = uart_read_bytes(uartPort, data, bufferSize - 1, pdMS_TO_TICKS(100));
        for (uint8_t* ptr = data; ptr < data + len; ++ptr) { // Note: Corrected 'length' to 'len'
            auto result = decoder.consume(*ptr);
            switch (result.type) {
                case GNSSDecoder::ResultType::NMEA:
                    if (auto nmeaDecoder = *std::get<std::optional<NMEADecoder*>>(result.data)) {
                        ESP_LOGI("GPSReader", "topic %s", nmeaDecoder->getTopic().c_str());
                        std::vector<std::string> items = nmeaDecoder->getItems();
                        std::string combinedItems;
                        for (auto it = items.begin(); it != items.end(); ++it) {
                            combinedItems += *it;
                            if (it != items.end() - 1) {
                                combinedItems += ", ";
                            }
                        }
                        ESP_LOGI("GPSReader", "%s", combinedItems.c_str());
                        ESP_LOGI("GPSReader", "result %s", nmeaDecoder->getMessage().c_str());
                    }
                    break;
                case GNSSDecoder::ResultType::UBX:
                    {
                        auto ubxDecoder = *std::get<std::optional<UBXDecoder*>>(result.data);
                        ESP_LOGI("GPSReader", "UBX");
                        // Use ubxDecoder to access the UBXDecoder object
                    }
                    break;
                case GNSSDecoder::ResultType::NO_RESULT:
                    // Optionally log that no result was processed yet
                    //ESP_LOGI("GPSReader", "No result processed yet.");
                    break;
            }
        }
    }
    delete[] data; // Cleanup, though this line is technically unreachable
}

