#pragma once

#include "esp_http_server.h"
#include <vector>
#include <cstdint>
#include <string>


struct UBXMessage {
    uint8_t class_id = 0;
    uint8_t message_id = 0;
    std::vector<uint8_t> data;
    std::string toJson() const;
    static UBXMessage fromHttpRequest(httpd_req_t* req);
};
