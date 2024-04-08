#pragma once
#include <vector>
#include "Counter.h"

class CircularBuffer {
public:
    explicit CircularBuffer(size_t capacity=10);
    ~CircularBuffer();

    void putFront(const MeasurementData& item);
    std::vector<MeasurementData> getMeasurementDatasGreaterThanSequence(int sequence);
    size_t size() const;

private:
    size_t capacity_;
    std::vector<MeasurementData> buffer_;
    size_t head_, tail_;
    bool full_;
};
