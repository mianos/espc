#include <string>
#include <memory>
#include <cJSON.h>
#include <esp_log.h>

#include "esp_log.h"
#include "PayloadExtractor.h"


static const char* TAG = "PayloadExtract";

// Conversion function implementations
void convert_to_float(const double input, void* output) {
    *static_cast<float*>(output) = static_cast<float>(input);
}

void convert_to_int(const double input, void* output) {
    *static_cast<int*>(output) = static_cast<int>(input);
}

// The main function to extract and convert values from a payload.
esp_err_t extract_value_from_payload(httpd_req_t *req, const char *fieldName, void* outValue, ValueConverter converter) {
    if (req == nullptr || fieldName == nullptr || outValue == nullptr) {
        ESP_LOGE(TAG, "Invalid arguments to extract_value_from_payload");
        return ESP_FAIL;
    }

    std::string buf;
    buf.resize(req->content_len);
    int read_len = httpd_req_recv(req, buf.data(), req->content_len);
    if (read_len <= 0) {
        ESP_LOGE(TAG, "Error reading request payload");
        return ESP_FAIL;
    }

    buf.resize(read_len);
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(buf.c_str()), cJSON_Delete);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *field = cJSON_GetObjectItem(root.get(), fieldName);
    if (!field || !cJSON_IsNumber(field)) {
        ESP_LOGE(TAG, "Failed to extract '%s' as a number", fieldName);
        return ESP_FAIL;
    }

    converter(field->valuedouble, outValue);
    return ESP_OK;
}
