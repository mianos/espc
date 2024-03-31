#pragma once

#include <string>
#include <vector>

class NMEADecoder {
private:
    enum class State;

    State state;
    std::string message;
    std::string checksum;
    std::string topic;
    std::vector<std::string> items;

public:
    NMEADecoder();
    bool consume(unsigned char c);
    void processMessage();
    std::string getMessage() const;
    std::string getChecksum() const;
    std::string getTopic() const;
    std::vector<std::string> getItems() const;
};