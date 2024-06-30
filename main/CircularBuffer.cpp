#include "CircularBuffer.h"

//static const char* TAG = "CircularBuffer";

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
    // Update the highest sequence number after adding a new item.
    if (item.sequenceNumber > highestSequenceNumber) {
        highestSequenceNumber = item.sequenceNumber;
    }
}


std::optional<std::pair<std::vector<MeasurementData>, bool>> CircularBuffer::getMeasurementDatasGreaterThanSequence(int sequence) {
    MutexLock lock(bufferMutex);
    std::vector<MeasurementData> result;

    // Check if the requested sequence is already higher than any known sequence number
    if (sequence > highestSequenceNumber + 1) {
        return std::make_optional(std::make_pair(result, true)); // Indicate the sequence is too high
    }

    size_t count = size();
    size_t index = tail_;
    while (count--) {
        if (buffer_[index].sequenceNumber >= sequence) {
            result.push_back(buffer_[index]);
        }
        index = (index + 1) % capacity_;
    }

    // The second part of the pair is false, indicating there are items with higher sequence numbers
    return std::make_optional(std::make_pair(result, false));
}


size_t CircularBuffer::size() const {
    if (full_) return capacity_;
    if (head_ >= tail_) return head_ - tail_;
    return capacity_ + head_ - tail_;
}

bool CircularBuffer::isDataAvailableForSequence(int sequence) {
    MutexLock lock(bufferMutex);

    if (sequence > highestSequenceNumber) {
        return false;
    }

    size_t count = size();
    size_t index = tail_;
    while (count--) {
        if (buffer_[index].sequenceNumber >= sequence) {
            return true;
        }
        index = (index + 1) % capacity_;
    }

    return false;
}
