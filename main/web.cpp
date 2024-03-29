#include <algorithm> 
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

void WebServer::start() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;

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
