#pragma once

#include "NMEADecoder.h"
#include "UBXDecoder.h"
#include <optional>
#include <variant>

class GNSSDecoder {
public:
    enum class ResultType {
        NO_RESULT,
        NMEA,
        UBX
    };

    struct Result {
        ResultType type;
        std::variant<std::optional<NMEADecoder*>, std::optional<UBXDecoder*>, std::monostate> data;
    };

private:
    enum class State;

    State state;
    NMEADecoder nmeaDecoder;
    UBXDecoder ubxDecoder;

    std::string nmeaMessage;
    std::string nmeaChecksum;
    std::vector<uint8_t> ubxMessage;

public:
    GNSSDecoder();
    Result consume(unsigned char c);
};