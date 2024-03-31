#pragma once

#include <string>
#include <vector>
#include <cstdint> // For uint8_t

class NMEADecoder {
public:
    NMEADecoder();
    void parseData(const std::string& data);

private:

#include <string>
#include <vector>

class NMEADecoder {
public:
    NMEADecoder() = default;
    void parseData(const std::string& data);

private:
    enum class UpperState {
        WAIT_FOR_START,
        DECODING_NMEA,
        DECODING_UBX
    } upperState{UpperState::WAIT_FOR_START};

    enum class NMEAState {
        READ_IDENTIFIER,
        COLLECT_FIELDS,
        READ_CHECKSUM,
        VALIDATE_AND_EXECUTE
    } nmeaState{NMEAState::READ_IDENTIFIER};

    enum class UBXState {
        SYNC2,
        CLASS,
        ID,
        LENGTH_LOW,
        LENGTH_HIGH,
        PAYLOAD,
        CHECKSUM_A,
        CHECKSUM_B
    } ubxState{UBXState::SYNC2};

    // Example method declarations for the decode functions
    UpperState decodeNMEA(char character);
    UpperState decodeUBX(char character);

    // Additional private members and methods...

    enum ProtocolType {
        UNKNOWN, // Default state
        NMEA,    // For NMEA protocol
        UBX      // For UBX protocol
    };

    ProtocolType currentProtocol; // Add this line to declare the current protocol
    
    // NMEA Parsing Members
    std::vector<std::string> nmeaFields;
    std::string nmeaCurrentField;
    std::string nmeaChecksum;

    // UBX Parsing Members
    std::vector<uint8_t> ubxPayload;
    uint8_t ubxClass, ubxID, ubxChecksumA, ubxChecksumB;
    uint16_t ubxLength;
    size_t ubxPayloadCounter;


    void resetNMEADecoder();
    void resetUBXDecoder();

    bool validateChecksum(const std::string& identifier, const std::vector<std::string>& fields, const std::string& checksum);
    void processSentence(const std::string& identifier, const std::vector<std::string>& fields);
    void processUBXMessage(uint8_t msgClass, uint8_t msgID, const std::vector<uint8_t>& payload);
};
