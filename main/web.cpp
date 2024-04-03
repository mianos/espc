#include <algorithm> 
#include "esp_log.h"
#include "cJSON.h"
#include "UBXMessage.h"
#include "web.h"
#include <string>
#include <memory>


#define GET_CONTEXT(req, ctx) \
    auto* ctx = static_cast<WebContext*>(req->user_ctx); \
    if (!ctx) { \
		ESP_LOGE(TAG,"ctx null?"); \
        httpd_resp_send_500(req); \
        return ESP_FAIL; \
    }

static const char* TAG = "Web";

#include "esp_log.h"
#include "cJSON.h"
#include <string>
#include <memory>

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

httpd_uri_t WebServer::getUri = {
    .uri = "/getCounter",
    .method = HTTP_GET,
    .handler = getHandler,
    .user_ctx = nullptr
};

httpd_uri_t WebServer::postUri = {
    .uri = "/postValues",
    .method = HTTP_POST,
    .handler = postHandler,
    .user_ctx = nullptr
};

WebServer::WebServer(uint16_t port) : port(port) {}

WebServer::~WebServer() {
    stop();
}


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

void WebServer::start(WebContext *ctx) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;

    if (httpd_start(&server, &config) == ESP_OK) {
		dac_post_uri.user_ctx = ctx;
        httpd_register_uri_handler(server, &dac_post_uri);
		getUri.user_ctx = ctx;
        httpd_register_uri_handler(server, &getUri);
		postUri.user_ctx = ctx;
        httpd_register_uri_handler(server, &postUri);
		ubx_post_uri.user_ctx = ctx;
		httpd_register_uri_handler(server, &ubx_post_uri);
    }
}

void WebServer::stop() {
    if (server) {
        httpd_stop(server);
    }
}

esp_err_t WebServer::getHandler(httpd_req_t *req) {
    int counterValue = getHardwareCounterValue();
    std::string response = "Counter Value: " + std::to_string(counterValue);
    httpd_resp_send(req, response.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t WebServer::postHandler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, std::min(static_cast<size_t>(remaining), sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        // Assume the received data is a plain integer for simplicity
        int value = std::stoi(std::string(buf, ret));
        setHardwareCounterValue(value);
        remaining -= ret;
    }

    httpd_resp_send(req, "POST value updated", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static int value = 42;

void WebServer::setHardwareCounterValue(int value) {
    // Set the hardware counter to the specified value
    // Placeholder for actual hardware interaction
	printf("Setting %d\n", value);
}

int WebServer::getHardwareCounterValue() {
    // Get the current value of the hardware counter
    // Placeholder for actual hardware interaction
    return value; // Example value
}
