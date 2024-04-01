#include "esp_log.h"
#include "UBXProtocol.h"


enum class UBXProtocol::State {
    ReadSync1 = 0,
    ReadSync2 = 1,
    ReadClass = 2,
    ReadID = 3,
    ReadLength1 = 4,
    ReadLength2 = 5,
    ReadPayload = 6,
    ReadCK_A = 7,
    ReadCK_B
};

UBXProtocol::UBXProtocol() : state(State::ReadSync1), length(0), ck_a(0), ck_b(0) {}

bool UBXProtocol::consume(unsigned char c) {
    switch (state) {
        case State::ReadSync1:
            if (c == 0xB5) {
                message.clear();
                length = 0;
                ck_a = 0;
                ck_b = 0;
                state = State::ReadSync2;
            }
            break;
        case State::ReadSync2:
            if (c == 0x62) {
                state = State::ReadClass;
            } else {
                state = State::ReadSync1;
            }
            break;
        case State::ReadClass:
            message.push_back(c);
            ck_a += c;
            ck_b += ck_a;
            state = State::ReadID;
            break;
        case State::ReadID:
            message.push_back(c);
            ck_a += c;
            ck_b += ck_a;
            state = State::ReadLength1;
            break;
        case State::ReadLength1:
            length = c;
            ck_a += c;
            ck_b += ck_a;
            state = State::ReadLength2;
            break;
        case State::ReadLength2:
            length |= ((uint16_t)c << 8);
            length -= 2;    // subtract the checksum
            ck_a += c;
            ck_b += ck_a;
            if (length > 0) {
                state = State::ReadPayload;
            } else {
                state = State::ReadCK_A;
            }
            break;
        case State::ReadPayload:
            message.push_back(c);
            ck_a += c;
            ck_b += ck_a;
            if (--length == 0) {
                state = State::ReadCK_A;
            }
            break;
		case State::ReadCK_A:
			if (c == ck_a) {
				state = State::ReadCK_B;
			} else {
				ESP_LOGE("GPSReader", "Check A not correct c %d want %d", static_cast<int>(c), static_cast<int>(ck_a));
				state = State::ReadSync1;
			}
			break;
		case State::ReadCK_B:
			if (c == ck_b) {
				state = State::ReadSync1;
				return true;
			} else {
				ESP_LOGE("GPSReader", "Check B not correct c %d want %d", static_cast<int>(c), static_cast<int>(ck_b));
				state = State::ReadSync1;
			}
			break;
		default:
			ESP_LOGI("UBXDecoder", "got unknown state %d", (int)state);
			return false;
    }
    return false;
}

std::vector<uint8_t> UBXProtocol::getMessage() const {
    return message;
}

std::string UBXProtocol::getChecksum() const {
    return std::string({static_cast<char>(ck_a), static_cast<char>(ck_b)});
}


void UBXProtocol::calculateChecksum(std::vector<uint8_t>& message) {
    uint8_t ck_a = 0, ck_b = 0;
    // Start checksum calculation from message class (skip sync chars)
    for (size_t i = 2; i < message.size(); i++) {
        ck_a += message[i];
        ck_b += ck_a;
    }
    message.push_back(ck_a);
    message.push_back(ck_b);
}