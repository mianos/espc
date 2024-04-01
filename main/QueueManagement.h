#pragma once
#include <queue>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "UBXMessage.h"

class QueueManager {
public:
    QueueManager();
    ~QueueManager();
    void postMessage(std::unique_ptr<UBXMessage> message);
    std::unique_ptr<UBXMessage> getMessage(TickType_t waitTime = portMAX_DELAY);

private:
    QueueHandle_t ubxQueue;
    static constexpr size_t queueSize = 10; // Adjust based on needs
};
