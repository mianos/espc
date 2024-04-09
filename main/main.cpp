#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string>

#include "wifimanager.h"
#include "web.h"
#include "gps.h"
#include "QueueManagement.h"
#include "Counter.h"
#include "CircularBuffer.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

void	serial_0_init() {
    // Configure UART parameters
    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
}


extern "C" void app_main() {
	WiFiManager wifiManager;
//    wifiManager.clearWiFiCredentials();
    wifiManager.initializeWiFi();
	CircularBuffer dbuf;

	auto cnt = Counter(dbuf);
	QueueManager queueManager;
	DAC1220 dac;
	dac.begin();
	WebContext wc(&queueManager, &dac, dbuf);
	WebServer webServer(wc); // Specify the web server port
//	ESP_LOGI("GPSReader", "started");
//	GPSReader gpsReader(UART_NUM_1, 4, 5, 9600);
//    gpsReader.initialize();
//    gpsReader.startReadLoopTask();
//	esp_log_level_set("*", ESP_LOG_INFO);
	//serial_0_init();
//    gpsReader.startPassThroughTask();
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100)); 
	
		cnt.process_count_queue();
	}
}

