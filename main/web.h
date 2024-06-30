#pragma once
#include <string>
#include "esp_log.h"
#include "esp_http_server.h"

#include "QueueManagement.h"
#include "CircularBuffer.h"
#include "Counter.h"
//#include "dac1220.h"

struct WebContext {
	QueueManager* queueManager;	// queue for outgoing on demand UBX messages
	//DAC1220 *dac;
	CircularBuffer& dbuf;
	Counter& cnt;
};

class WebServer {
public:
    WebServer(WebContext &wc, uint16_t port=80);

private:
    httpd_handle_t server = nullptr;
    static esp_err_t getHandler(httpd_req_t *req);
    static esp_err_t postHandler(httpd_req_t *req);
    static httpd_uri_t getUri;
    static httpd_uri_t postUri;
    uint16_t port;
};
