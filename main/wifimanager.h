#pragma once

#include "nv.h" // Include the NvsStorageManager header
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_smartconfig.h"

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();

    void initializeWiFi();
    void clearWiFiCredentials(); // Method to clear WiFi credentials

private:
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void smartConfigTask(void* param);

    static NvsStorageManager storageManager; // Static instance of NvsStorageManager
    static EventGroupHandle_t wifi_event_group;
    static constexpr int CONNECTED_BIT = BIT0;
    static constexpr int ESPTOUCH_DONE_BIT = BIT1;
    static const char* TAG;
};