#include "nvs_flash.h"
#include "esp_log.h"
#include <string>

#include "wifimanager.h"
#include "web.h"
#include "gps.h"

extern "C" void counter_init();
extern "C" void init_ledc_square_wave(void);
extern "C" void display_count(void);

extern "C" void app_main() {
    // Initialize the WiFiManager
	WiFiManager wifiManager;
    wifiManager.initializeWiFi();
//    ESP_LOGI("app_main", "Clearing stored WiFi credentials...");    wifiManager.clearWiFiCredentials();

	init_ledc_square_wave();
	counter_init();
	WebServer webServer(80); // Specify the web server port
    webServer.start();
	printf("started\n");
	GPSReader gpsReader(UART_NUM_1, 12, 13, 9600);
    gpsReader.initialize();
    gpsReader.startReadLoopTask();
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
