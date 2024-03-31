#include "GNSSDecoder.h"
#include <iostream>

enum class GNSSDecoder::State {
    WaitForStart,
    DecodeNMEA,
    DecodeUBX
};

GNSSDecoder::GNSSDecoder() : state(State::WaitForStart) {}

GNSSDecoder::Result GNSSDecoder::consume(unsigned char c) {
    switch (state) {
        case State::WaitForStart:
            if (c == '$') {
                state = State::DecodeNMEA;
                nmeaDecoder.consume(c);
            } else if (c == 0xB5) {
                state = State::DecodeUBX;
                ubxDecoder.consume(c);
            }
            break;
        case State::DecodeNMEA:
            if (nmeaDecoder.consume(c)) {
                std::cout << "nmea message" << std::endl;
                state = State::WaitForStart;
                return {ResultType::NMEA, std::make_optional(&nmeaDecoder)};
            }
            break;
        case State::DecodeUBX:
            if (ubxDecoder.consume(c)) {
                std::cout << "UBX message" << std::endl;
                state = State::WaitForStart;
                return {ResultType::UBX, std::make_optional(&ubxDecoder)};
            }
            break;
    }
    return {ResultType::NO_RESULT, std::monostate{}};
}
