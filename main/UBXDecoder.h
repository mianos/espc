#pragma once

#include <vector>
#include <cstdint>
#include <string>

class UBXDecoder {
private:
    enum class State;

    State state;
    std::vector<uint8_t> message;
    uint16_t length;
    uint8_t ck_a, ck_b;

public:
    UBXDecoder();
    bool consume(unsigned char c);
    std::vector<uint8_t> getMessage() const;
    std::string getChecksum() const;
};