#include "nvs_flash.h"
#include "esp_log.h"
#include <string>

#include "wifimanager.h"

extern "C" void counter_init();
extern "C" void init_ledc_square_wave(void);
extern "C" void display_count(void);

extern "C" void app_main() {
    // Initialize the WiFiManager
	WiFiManager wifiManager;
    wifiManager.initializeWiFi();

	init_ledc_square_wave();
	counter_init();
    // Example usage: Uncomment the following lines if you want to clear
    // stored WiFi credentials for testing or before starting SmartConfig.
    // Note: Be sure to comment these back or remove after testing to prevent
    // clearing credentials on every device restart.
    //
    ESP_LOGI("app_main", "Clearing stored WiFi credentials...");
//    wifiManager.clearWiFiCredentials();
    // After clearing credentials, the device will need to be provisioned again
    // via SmartConfig or manually setting the credentials.

    // At this point, the WiFiManager will attempt to connect using stored
    // credentials. If no credentials are stored, it will fall back to
    // SmartConfig to provision the device.

    // Your application logic here
    // For example, you might enter a main loop, start other tasks,
    // or wait for events.
	printf("started\n");
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
