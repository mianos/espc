set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

idf_component_register(
    SRCS
        main.cpp
        Counter.cpp
        web.cpp
        gps.cpp
        GNSSDecoder.cpp
        NMEADecoder.cpp
        UBXProtocol.cpp
        UBXMessage.cpp
        QueueManagement.cpp
        CircularBuffer.cpp
		PayloadExtractor.cpp
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        esp_timer
        wifimanager
        nvsstoragemanager
        jsonwrapper
        esp_http_server
)
