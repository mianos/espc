#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class MutexLock {
public:
    explicit MutexLock(SemaphoreHandle_t& mutex)
    : mutex_(mutex) {
        xSemaphoreTake(mutex_, portMAX_DELAY); // Lock the mutex
    }

    ~MutexLock() {
        xSemaphoreGive(mutex_); // Unlock the mutex on destruction
    }

    // Delete copy constructor and copy assignment operator to prevent copying
    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;

private:
    SemaphoreHandle_t& mutex_;
};
