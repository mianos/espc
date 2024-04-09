#include "CircularBuffer.h"

CircularBuffer::CircularBuffer(size_t capacity)
    : capacity_(capacity), buffer_(capacity), head_(0), tail_(0), full_(false) {
		 bufferMutex = xSemaphoreCreateMutex();
}

CircularBuffer::~CircularBuffer() {
	vSemaphoreDelete(bufferMutex);
}


void CircularBuffer::putFront(const MeasurementData& item) {
	MutexLock lock(bufferMutex);

    if (full_) {
        // Overwrite the oldest item if the buffer is full
        tail_ = (tail_ + 1) % capacity_;
    }
    buffer_[head_] = item;
    head_ = (head_ + 1) % capacity_;
    full_ = head_ == tail_;
}

std::vector<MeasurementData> CircularBuffer::getMeasurementDatasGreaterThanSequence(int sequence) {
    MutexLock lock(bufferMutex);

    std::vector<MeasurementData> result;
    size_t count = size();
    size_t index = tail_;
    while (count--) {
        if (buffer_[index].sequenceNumber > sequence) {
            result.push_back(buffer_[index]);
        }
        index = (index + 1) % capacity_;
    }
    // Removed the conditional block that adds a default MeasurementData object
    return result;
}


size_t CircularBuffer::size() const {
    if (full_) return capacity_;
    if (head_ >= tail_) return head_ - tail_;
    return capacity_ + head_ - tail_;
}
