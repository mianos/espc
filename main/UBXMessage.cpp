
#include "UBXMessage.h"
#include "esp_http_server.h"
#include "cJSON.h"

std::string UBXMessage::toJson() const {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "class", class_id);
    cJSON_AddNumberToObject(root, "message_id", message_id);

    cJSON* dataArray = cJSON_AddArrayToObject(root, "data");
    for (auto byte : data) {
        cJSON_AddItemToArray(dataArray, cJSON_CreateNumber(byte));
    }

    char* rawJson = cJSON_Print(root);
    std::string jsonString(rawJson);
    cJSON_free(rawJson);
    cJSON_Delete(root);

    return jsonString;
}

UBXMessage UBXMessage::fromHttpRequest(httpd_req_t* req) {
    UBXMessage message;

    if (req->content_len <= 0) {
        return message; // Early return on empty request
    }

    std::vector<char> buffer(req->content_len + 1, '\0'); // +1 for null termination
    int ret = httpd_req_recv(req, buffer.data(), req->content_len);
    if (ret <= 0) {
        return message; // Early return on read error
    }

    cJSON* root = cJSON_Parse(buffer.data());
    if (root == nullptr) {
        cJSON_Delete(root);
        return message; // Early return on parse error
    }

    cJSON* classId = cJSON_GetObjectItemCaseSensitive(root, "class");
    cJSON* messageId = cJSON_GetObjectItemCaseSensitive(root, "message_id");
    cJSON* data = cJSON_GetObjectItemCaseSensitive(root, "data");

    if (cJSON_IsNumber(classId) && cJSON_IsNumber(messageId) && cJSON_IsArray(data)) {
        message.class_id = static_cast<uint8_t>(classId->valuedouble);
        message.message_id = static_cast<uint8_t>(messageId->valuedouble);

        cJSON* dataByte;
        cJSON_ArrayForEach(dataByte, data) {
            if (cJSON_IsNumber(dataByte)) {
                message.data.push_back(static_cast<uint8_t>(dataByte->valuedouble));
            }
        }
    }

    cJSON_Delete(root);

    return message;
}
