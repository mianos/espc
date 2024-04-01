#include <algorithm> 
#include "QueueManagement.h"
#include "UBXMessage.h"
#include "web.h"

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
    auto* queueManager = static_cast<QueueManager*>(req->user_ctx);
    if (!queueManager) {
        // If for some reason the queueManager is not available, send an internal server error
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    queueManager->postMessage(std::make_unique<UBXMessage>(std::move(message)));
    httpd_resp_send(req, nullptr, 0); // No additional data to send
    return ESP_OK;
}

void WebServer::start(QueueManager* queueManager) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;

	httpd_uri_t ubx_post_uri = {
		.uri = "/ubx",
		.method = HTTP_POST,
		.handler = post_ubx_handler,
		.user_ctx = queueManager // Pass the QueueManager instance here
	};
	httpd_register_uri_handler(server, &ubx_post_uri);



    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &getUri);
        httpd_register_uri_handler(server, &postUri);
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
