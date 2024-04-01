#pragma once

#include <vector>
#include <cstdint>
#include <string>

class UBXProtocol {
private:
    enum class State;

    State state;
    std::vector<uint8_t> message;
    uint16_t length;
    uint8_t ck_a, ck_b;

public:
    UBXProtocol();
    bool consume(unsigned char c);
    std::vector<uint8_t> getMessage() const;
    std::string getChecksum() const;
    void calculateChecksum(std::vector<uint8_t>& message);
};