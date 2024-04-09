#pragma once
#include "cJSON.h"
#include <string>

struct MeasurementData {
    int64_t timerCount = 0;
    int counterValue = 0;
    int sequenceNumber = 0;

    std::string toJsonString() const {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "timerCount", this->timerCount);
        cJSON_AddNumberToObject(root, "counterValue", this->counterValue);
        cJSON_AddNumberToObject(root, "sequenceNumber", this->sequenceNumber);
        char *out = cJSON_Print(root);
        std::string jsonStr(out);
        cJSON_free(out);
        cJSON_Delete(root);
        return jsonStr;
    }
};
