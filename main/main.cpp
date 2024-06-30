#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string>

#include "WifiManager.h"
#include "SettingsManager.h"

#include "web.h"
#include "gps.h"
#include "QueueManagement.h"
#include "Counter.h"
#include "CircularBuffer.h"

static const char *TAG = "gpsdo";

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

static SemaphoreHandle_t wifiSemaphore;

static void localEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
	    xSemaphoreGive(wifiSemaphore);
	}
}

extern "C" void app_main() {
	NvsStorageManager nv;
	SettingsManager settings(nv);

	wifiSemaphore = xSemaphoreCreateBinary();
	WiFiManager wifiManager(nv, localEventHandler, nullptr);
//	wifiManager.clear();

	CircularBuffer dbuf;

	auto cnt = Counter(dbuf);
	QueueManager queueManager;
//	DAC1220 dac;
//	dac.begin();
	//WebContext wc(&queueManager, &dac, dbuf, cnt);
	WebContext wc(&queueManager, dbuf, cnt);
	WebServer webServer(wc); // Specify the web server port
	ESP_LOGI("GPSReader", "started");
 	//uart, tx, rx 
	GPSReader gpsReader(UART_NUM_1, 19, 17, 9600);
    gpsReader.initialize();
    gpsReader.startReadLoopTask();
//	esp_log_level_set("*", ESP_LOG_INFO);
//   gpsReader.startPassThroughTask();
    if (xSemaphoreTake(wifiSemaphore, portMAX_DELAY) ) {
		ESP_LOGI(TAG, "Main task continues after WiFi connection.");
		while (true) {
			vTaskDelay(pdMS_TO_TICKS(100)); 
		
			cnt.process_count_queue();
		}
	}
}

