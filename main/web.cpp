#include <algorithm> 
#include <regex>
#include "esp_log.h"
#include "cJSON.h"
#include "UBXMessage.h"
#include "web.h"
#include <string>
#include <memory>
#include "CircularBuffer.h"
#include "PayloadExtractor.h"


#define GET_CONTEXT(req, ctx) \
    auto* ctx = static_cast<WebContext*>(req->user_ctx); \
    if (!ctx) { \
		ESP_LOGE(TAG,"ctx null?"); \
        httpd_resp_send_500(req); \
        return ESP_FAIL; \
    }

static const char* TAG = "Web";

std::string get_cookie_header(httpd_req_t *request) {
    size_t header_value_length = httpd_req_get_hdr_value_len(request, "Cookie") + 1;
    if (header_value_length > 1) {
        std::string cookie_value(header_value_length, '\0');
        if (httpd_req_get_hdr_value_str(request, "Cookie", cookie_value.data(), header_value_length) == ESP_OK) {
            return cookie_value;
        } else {
            ESP_LOGE(TAG, "Failed to get Cookie header");
        }
    } else {
        ESP_LOGE(TAG, "Cookie header not found");
    }
    return std::string(); // Return an empty string if the cookie is not found or an error occurred
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
    esp_err_t result = extract_value_from_payload(req, "voltage", &voltage, convert_to_float);
    if (result == ESP_OK) {
        ctx->dac->setVoltage(voltage);
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


// Function to parse the sequence number from the cookie
int parseSequenceNumber(const std::string& cookie) {
    std::regex seqRegex("SeqNum=(\\d+)");
    std::smatch matches;
    if (std::regex_search(cookie, matches, seqRegex) && matches.size() > 1) {
        return std::stoi(matches[1].str());
    }
    return -1; // Indicates no valid sequence number found
}



int get_max_wait_parameter(httpd_req_t *req) {
    char query[128];
    char value[32];
    int max_wait = -1;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "max_wait", value, sizeof(value)) == ESP_OK) {
            max_wait = atoi(value);
        }
    }
    return max_wait;
}

esp_err_t WebServer::getHandler(httpd_req_t *req) {
	GET_CONTEXT(req, ctx);
    auto cookie = get_cookie_header(req);
    auto seqNum = parseSequenceNumber(cookie);
    if (seqNum < 0) {
		ESP_LOGI(TAG, "Invalid sequence number: %d", seqNum);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

	auto startTime = esp_timer_get_time(); // Start time in microseconds
	auto max_wait = get_max_wait_parameter(req);
	auto endTime = startTime;
	if (max_wait != -1) {
		endTime = startTime + (max_wait * 1000000); // End time in microseconds
	}
	while (esp_timer_get_time() < endTime) {
		if (ctx->dbuf.isDataAvailableForSequence(seqNum)) {
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}

    auto result = ctx->dbuf.getMeasurementDatasGreaterThanSequence(seqNum);

	if (result->second) {
		// Reset the sequence number
		ESP_LOGI(TAG, "Resetting sequence number on account of cookie being %d", seqNum);
		std::string newCookie = "SeqNum=0; Path=/; Max-Age=3600";
		httpd_resp_set_hdr(req, "Set-Cookie", newCookie.c_str());
		httpd_resp_set_status(req, "204 No Content");
		httpd_resp_send(req, nullptr, 0); // No body
		return ESP_OK;
	}
	auto datas = result->first;
	if (!datas.empty()) {
        seqNum = datas.back().sequenceNumber + 1; // The last element holds the highest sequence number.
    }
    std::string response = "[";
    for (size_t i = 0; i < datas.size(); ++i) {
        response += datas[i].toJsonString();
        if (i < datas.size() - 1) {
            response += ",";
        }
    }
	response += "]";
    // Set the updated sequence number in the response cookie
    std::string newCookie = "SeqNum=" + std::to_string(seqNum) + "; Path=/; Max-Age=3600";
    httpd_resp_set_hdr(req, "Set-Cookie", newCookie.c_str());
    httpd_resp_send(req, response.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t WebServer::getUri = {
    .uri = "/getCounter",
    .method = HTTP_GET,
    .handler = getHandler,
    .user_ctx = nullptr
};


esp_err_t set_period(httpd_req_t *req) {
	GET_CONTEXT(req, ctx);
    int period;
    if (extract_value_from_payload(req, "period", &period, convert_to_int) == ESP_OK) {
		ctx->cnt.set_loops(period);
    } else {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
	httpd_resp_send(req, nullptr, 0); // No additional data to send
	return ESP_OK;
}

httpd_uri_t post_set_period = {
	.uri = "/counter",
	.method = HTTP_POST,
	.handler = set_period,
	.user_ctx = nullptr
};

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
		post_set_period.user_ctx = &ctx;
		httpd_register_uri_handler(server, &post_set_period);
    }
}
