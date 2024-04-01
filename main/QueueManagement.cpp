#include "QueueManagement.h"

QueueManager::QueueManager() {
    this->ubxQueue = xQueueCreate(queueSize, sizeof(std::unique_ptr<UBXMessage>));
}

QueueManager::~QueueManager() {
    std::unique_ptr<UBXMessage> message;
    while (xQueueReceive(this->ubxQueue, &message, 0)) {
        // Unique_ptr automatically frees memory
    }
    vQueueDelete(this->ubxQueue);
}

void QueueManager::postMessage(std::unique_ptr<UBXMessage> message) {
    if (xQueueSend(this->ubxQueue, &message, portMAX_DELAY) != pdPASS) {
        // Handle failure (message will be automatically deleted)
    }
}

std::unique_ptr<UBXMessage> QueueManager::getMessage(TickType_t waitTime) {
    std::unique_ptr<UBXMessage> message;
    if (xQueueReceive(this->ubxQueue, &message, waitTime)) {
        return message;
    }
    return nullptr;
}
