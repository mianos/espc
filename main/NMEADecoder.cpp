#include "esp_log.h"
#include "NMEADecoder.h"

enum class NMEADecoder::State {
    Start,
    ReadMessage,
    ReadChecksum1,
    ReadChecksum2
};

NMEADecoder::NMEADecoder() : state(State::Start) {}

bool NMEADecoder::consume(unsigned char c) {
    switch (state) {
        case State::Start:
            if (c == '$') {
                message.clear();
                checksum.clear();
                topic.clear();
                items.clear();
                state = State::ReadMessage;
            } else {
                // ignore
            }
            return false;
        case State::ReadMessage:
            if (c == '*') {
                state = State::ReadChecksum1;
            } else {
                message += c;
            }
            return false;
        case State::ReadChecksum1:
            checksum += c;
            state = State::ReadChecksum2;
            return false;
        case State::ReadChecksum2:
            checksum += c;
            processMessage();
            state = State::Start;
            return true;
		default:
			ESP_LOGI("NMEADecoder", "got unknown state %d", (int)state);
			return false;

    }
}

void NMEADecoder::processMessage() {
    enum class State {
        Topic,
        Item,
        Done
    };

    State state = State::Topic;
    std::string currentItem;

    for (char c : message) {
        switch (state) {
            case State::Topic:
                if (c == ',') {
                    topic = currentItem;
                    currentItem.clear();
                    state = State::Item;
                } else {
                    currentItem += c;
                }
                break;
            case State::Item:
                if (c == ',') {
                    items.emplace_back(currentItem);
                    currentItem.clear();
                } else {
                    currentItem += c;
                }
                break;
            case State::Done:
                break;
        }
    }

    if (!currentItem.empty()) {
        if (state == State::Topic) {
            topic = currentItem;
        } else {
            items.emplace_back(currentItem);
        }
    }
}

std::string NMEADecoder::getMessage() const {
    return message;
}

std::string NMEADecoder::getChecksum() const {
    return checksum;
}

std::string NMEADecoder::getTopic() const {
    return topic;
}

std::vector<std::string> NMEADecoder::getItems() const {
    return items;
}
