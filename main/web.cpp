#include <algorithm> 
#include <regex>
#include "esp_log.h"
#include "cJSON.h"
#include "UBXMessage.h"
#include "web.h"
#include <string>
#include <memory>
#include "CircularBuffer.h"


#define GET_CONTEXT(req, ctx) \
    auto* ctx = static_cast<WebContext*>(req->user_ctx); \
    if (!ctx) { \
		ESP_LOGE(TAG,"ctx null?"); \
        httpd_resp_send_500(req); \
        return ESP_FAIL; \
    }

static const char* TAG = "Web";

/**
 * Extracts a float value from a JSON payload for a specified field using C++ idioms.
 * 
 * @param req The HTTP request containing the JSON payload.
 * @param fieldName The name of the field to extract the value from.
 * @param outValue Pointer to a float where the extracted value will be stored.
 * @return ESP_OK if extraction is successful, otherwise ESP_FAIL.
 */
esp_err_t extract_float_from_payload_cpp(httpd_req_t *req, const char *fieldName, float *outValue) {
    if (req == nullptr || fieldName == nullptr || outValue == nullptr) {
        ESP_LOGE(TAG, "Invalid arguments to extract_float_from_payload_cpp");
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
    *outValue = static_cast<float>(field->valuedouble);
    return ESP_OK;
}

std::string get_cookie_header(httpd_req_t *request) {
    size_t header_value_length = httpd_req_get_hdr_value_len(request, "Cookie") + 1;
    if (header_value_length > 1) {
        std::string cookie_value(header_value_length, '\0');
        if (httpd_req_get_hdr_value_str(request, "Cookie", cookie_value.data(), header_value_length) == ESP_OK) {
            return cookie_value;
        } else {
            ESP_LOGE("HTTP", "Failed to get Cookie header");
        }
    } else {
        ESP_LOGE("HTTP", "Cookie header not found");
    }
    return std::string(); // Return an empty string if the cookie is not found or an error occurred
}

httpd_uri_t WebServer::getUri = {
    .uri = "/getCounter",
    .method = HTTP_GET,
    .handler = getHandler,
    .user_ctx = nullptr
};


esp_err_t post_ubx_handler(httpd_req_t *req) {
    // Deserialize directly from the HTTP request
    UBXMessage message = UBXMessage::fromHttpRequest(req);
    if (message.data.empty()) { // Assuming an empty data vector indicates failure
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    auto* ctx = static_cast<WebContext*>(req->user_ctx);
    if (!ctx) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    ctx->queueManager->postMessage(std::make_unique<UBXMessage>(std::move(message)));
    httpd_resp_send(req, nullptr, 0); // No additional data to send
    return ESP_OK;
}

httpd_uri_t ubx_post_uri = {
	.uri = "/ubx",
	.method = HTTP_POST,
	.handler = post_ubx_handler,
	.user_ctx = nullptr
};

esp_err_t post_dac_handler(httpd_req_t *req) {
	GET_CONTEXT(req, ctx);
    float voltage;
    esp_err_t result = extract_float_from_payload_cpp(req, "voltage", &voltage);
    if (result == ESP_OK) {
        ctx->dac->set_voltage(voltage);
        ESP_LOGI(TAG, "DAC voltage set to %.4f", voltage);
    } else {
        ESP_LOGE(TAG, "Failed to set DAC voltage");
        // Handle error, perhaps send an error response
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
	httpd_resp_send(req, nullptr, 0); // No additional data to send
	return ESP_OK;
}

httpd_uri_t dac_post_uri = {
	.uri = "/dac",
	.method = HTTP_POST,
	.handler = post_dac_handler,
	.user_ctx = nullptr
};


// Placeholder for getEnd function
// Returns the next sequence number as an integer.
int getEnd() {
    // Implement this function to return the next sequence number.
    // This is just a placeholder implementation.
    static int counter = 0;
    return ++counter;
}

// Function to parse the sequence number from the cookie
int parseSequenceNumber(const std::string& cookie) {
    std::regex seqRegex("SeqNum=(\\d+)");
    std::smatch matches;
    if (std::regex_search(cookie, matches, seqRegex) && matches.size() > 1) {
        return std::stoi(matches[1].str());
    }
    return -1; // Indicates no valid sequence number found
}

esp_err_t WebServer::getHandler(httpd_req_t *req) {
	GET_CONTEXT(req, ctx);
    auto cookie = get_cookie_header(req);
    ESP_LOGI("WebServer", "Received cookie: '%s'", cookie.c_str());

    // Parse the sequence number from the cookie
    int seqNum = parseSequenceNumber(cookie);
    if (seqNum < 0) {
		ESP_LOGI("WebServer", "Invalid sequence number: %d", seqNum);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    std::string response = "[";
	
    auto datas = ctx->dbuf.getMeasurementDatasGreaterThanSequence(seqNum);
	if (!datas.empty()) {
        seqNum = datas.back().sequenceNumber; // The last element holds the highest sequence number.
    }
    for (size_t i = 0; i < datas.size(); ++i) {
        response += datas[i].toJsonString();
        if (i < datas.size() - 1) {
            response += ",";
        }
    }
	response += "]"; // Close JSON array
    // Set the updated sequence number in the response cookie
    std::string newCookie = "SeqNum=" + std::to_string(seqNum) + "; Path=/; Max-Age=3600";
    httpd_resp_set_hdr(req, "Set-Cookie", newCookie.c_str());
    httpd_resp_send(req, response.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


WebServer::WebServer(WebContext& ctx, uint16_t port) : port(port) {
	ESP_LOGI(TAG, "Starting web server  on port %d", port);
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;

    if (httpd_start(&server, &config) == ESP_OK) {
		dac_post_uri.user_ctx = &ctx;
        httpd_register_uri_handler(server, &dac_post_uri);
		getUri.user_ctx = &ctx;
        httpd_register_uri_handler(server, &getUri);
		ubx_post_uri.user_ctx = &ctx;
		httpd_register_uri_handler(server, &ubx_post_uri);
    }
}
