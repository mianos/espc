#pragma once
#include "esp_http_server.h"

typedef void (*ValueConverter)(const double, void*);

void convert_to_float(const double input, void* output);
void convert_to_int(const double input, void* output);
esp_err_t extract_value_from_payload(httpd_req_t *req,
		const char *fieldName, void* outValue, ValueConverter converter);
