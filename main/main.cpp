#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string>

#include "wifimanager.h"
#include "web.h"
#include "gps.h"
#include "QueueManagement.h"

extern "C" void counter_init();
extern "C" void init_ledc_square_wave(void);
extern "C" void display_count(void);

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
    uart_config.source_clk = UART_SCLK_APB;
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
}

static QueueManager queueManager;

extern "C" void app_main() {
    // Initialize the WiFiManager
	WiFiManager wifiManager;
    wifiManager.initializeWiFi();
//    ESP_LOGI("app_main", "Clearing stored WiFi credentials...");    wifiManager.clearWiFiCredentials();

	init_ledc_square_wave();
	counter_init();
	WebServer webServer(80); // Specify the web server port
    webServer.start(&queueManager);
	ESP_LOGI("GPSReader", "started");
	GPSReader gpsReader(UART_NUM_1, 2, 13, 9600);
    gpsReader.initialize();
    gpsReader.startReadLoopTask();
	//serial_0_init();
//    gpsReader.startPassThroughTask();
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
		display_count();
	}
}


#if 0
void app_main() {

	extern void init_ledc_square_wave();
	extern void start_pulse_timer();

	ESP_ERROR_CHECK(nvs_flash_init());
//	initialise_wifi();
    init_ledc_square_wave();
    start_pulse_timer();
	counter_init();
	
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
		int count_value;
        if (xQueueReceive(count_queue, &count_value, pdMS_TO_TICKS(300)) == pdPASS) {
			printf("counter %d\n", count_value); 
        }
    }
}
#endif
