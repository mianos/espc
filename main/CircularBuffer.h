#pragma once
#include <vector>
#include <optional>

#include "Measurement.h"
#include "Mutex.h"

class CircularBuffer {
public:
    explicit CircularBuffer(size_t capacity=10);
    ~CircularBuffer();

    void putFront(const MeasurementData& item);
	std::optional<std::pair<std::vector<MeasurementData>, bool>> getMeasurementDatasGreaterThanSequence(int sequence);

private:
    size_t size() const;
    size_t capacity_;
    std::vector<MeasurementData> buffer_;
    size_t head_, tail_;
    bool full_;
   SemaphoreHandle_t bufferMutex;
   int highestSequenceNumber = INT_MIN; // Initialize with the minimum possible value

};
