#ifndef TRAFFIC_LIGHT_NRF9160_HTTP_MODULE_H_
#define TRAFFIC_LIGHT_NRF9160_HTTP_MODULE_H_

#include <stdlib.h>
#include <cJSON.h>
#include "zephyr/kernel.h"

// Performs an HTTP GET request
// @param host - String representing the host name/IP address/domain name (ie. www.example.com or 8.8.8.8)
// @param url - String representing the URL path (ie. /index.html)
int get_request(char* host, char* url, const char** headers);

// Performs an HTTP POST request
// @param host - String representing the host name/IP address/domain name (ie. www.example.com or 8.8.8.8)
// @param url - String representing the URL path (ie. /index.html)
// @param payload - Payload to put in the POST request
// @param payload_size - Length of the payload in bytes
int post_request(char* host, char* url, char* payload, size_t payload_size, const char** headers);

void take_http_sem();
void give_http_sem();

cJSON* parse_json_response();
void free_json_response(cJSON* parsed_json);

char* get_http_rx_content();
size_t get_http_rx_content_length();

// We need to define these special functions for cJSON to call when doing malloc's and frees
void* cjson_alloc(size_t size);
void cjson_free(void* ptr);



#endif // TRAFFIC_LIGHT_NRF9160_HTTP_MODULE_H_