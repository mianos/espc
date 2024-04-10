#pragma once
#include "cJSON.h"
#include <string>

struct MeasurementData {
    int64_t timerCount = 0;
    uint64_t counterValue = 0;
    int sequenceNumber = 0;
	int period = 0;
	int overflows = 0;
    int rawCounterValue = 0;

    std::string toJsonString() const {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "timerCount", this->timerCount);
        cJSON_AddNumberToObject(root, "counterValue", this->counterValue);
        cJSON_AddNumberToObject(root, "sequenceNumber", this->sequenceNumber);
        cJSON_AddNumberToObject(root, "period", this->period);
        cJSON_AddNumberToObject(root, "overflows", this->overflows);
        char *out = cJSON_Print(root);
        std::string jsonStr(out);
        cJSON_free(out);
        cJSON_Delete(root);
        return jsonStr;
    }
};
