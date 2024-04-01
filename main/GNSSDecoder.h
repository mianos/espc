#pragma once

#include "NMEADecoder.h"
#include "UBXProtocol.h"
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
        std::variant<std::optional<NMEADecoder*>, std::optional<UBXProtocol*>, std::monostate> data;
    };

private:
    enum class State;

    State state;
    NMEADecoder nmeaDecoder;
    UBXProtocol ubxDecoder;

    std::string nmeaMessage;
    std::string nmeaChecksum;
    std::vector<uint8_t> ubxMessage;

public:
    GNSSDecoder();
    Result consume(unsigned char c);
};