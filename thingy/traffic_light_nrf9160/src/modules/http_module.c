/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include <string.h>
#include <stdlib.h>
#include <net/socket.h>
#include <net/net_ip.h>
#include <net/http_client.h>

#include <cJSON.h>

#define MODULE http_module
#include <caf/events/module_state_event.h>
#include "events/lte_event.h"
#include "events/ae_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define ENDPOINT_HOSTNAME "3.88.130.137"
#define ENDPOINT_PORT 80

#define HTTP_RX_BUF_SIZE 2048

static int connect_socket(const char *hostname_str, int port, int *sock);

bool http_connected = false;
// String representation of the resolved IP address
static char resolved_ip_addr[INET6_ADDRSTRLEN];
// Buffer that we store the HTTP response in
static char http_rx_buf[HTTP_RX_BUF_SIZE];
// A pointer into http_rx_buf that marks the start of the response body (ie. after the headers)
static char* http_rx_body_start = 0;
// Number of bytes long that the HTTP response body is
size_t http_content_length = 0;
// Socket used to make http connection
static int http_socket = -1;
// HTTP timeout is 10 seconds
static int32_t HTTP_REQUEST_TIMEOUT = 10 * MSEC_PER_SEC;

// Take this semaphore when making an HTTP request, and then give it back when done with the http_rx_buf data.
K_SEM_DEFINE(http_request_sem, 1, 1);

// Define a heap for parsing and constructing JSON objects using cJSON
K_HEAP_DEFINE(cjson_heap, 4096);

struct addrinfo *addr_res;
struct addrinfo addr_hints = {
	.ai_family = AF_INET,
	.ai_socktype = SOCK_STREAM
};

// We need to define these special functions for cJSON to call when doing malloc's and frees
void* cjson_alloc(size_t size) {
	return k_heap_alloc(&cjson_heap, size, K_FOREVER);
}

void cjson_free(void* ptr) {
	k_heap_free(&cjson_heap, ptr);
}

struct cJSON_Hooks cjson_mem_hooks = {
	.malloc_fn = cjson_alloc,
	.free_fn = cjson_free
};

/* Gets and stores the respones from an HTTP request
	This is taken from the zephyr http_client examples */
static void response_cb(struct http_response *rsp,
						enum http_final_call final_data,
						void *user_data)
{
	ARG_UNUSED(user_data);
	if (final_data == HTTP_DATA_MORE)
	{
		LOG_INF("Partial data received (%zd bytes)", rsp->data_len);
	}
	else if (final_data == HTTP_DATA_FINAL)
	{
		LOG_INF("All the data received (%zd bytes)", rsp->data_len);
	}

	LOG_INF("Response status %s", rsp->http_status);
}

static int get_request(char* url) {
		// !!! WARNING !!!
		//    Make sure to call k_sem_take(&http_request_sem, K_FOREVER) before calling this function, 
		//		and then call k_sem_give(&http_request_sem) when you are done with the http_rx_buf !!!!! 
		//

		if (!http_connected) {
			LOG_WRN("Attempted to call get_request %s when http_connected is false!", url);
			return -1;
		}

		// Clear out the response buffer
		memset(&http_rx_buf, 0, sizeof(http_rx_buf));
		// Create a new request
		struct http_request req;
		memset(&req, 0, sizeof(req));
		req.method = HTTP_GET;
		req.url = url;
		req.host = ENDPOINT_HOSTNAME;
		req.protocol = "HTTP/1.1";
		req.response = response_cb;
		req.recv_buf = &http_rx_buf[0];
		req.recv_buf_len = sizeof(http_rx_buf);

		int retry_count = 0;
		int response = 0;
		bool ok = false;
		while (retry_count < 3) {
			response = http_client_req(http_socket, &req, HTTP_REQUEST_TIMEOUT, NULL);
			if (response < 0) {
				LOG_ERR("http_client_req returned %d !", response);
				if (response == -128) {
					http_connected = false;
					int reconnect_response = connect_socket(ENDPOINT_HOSTNAME, ENDPOINT_PORT, &http_socket);
					if (reconnect_response < 0) {
						LOG_ERR("connect_socket() failed!");
						return -2;
					}
					else {
						LOG_INF("HTTP CONNECTED");
						http_connected = true;
					}
				}
				retry_count++;
				continue;
			}
			else {
				ok = true;
				break;
			}
		}
		if (!ok) {
			LOG_ERR("Retried %d times, quitting request!", retry_count);
			return -1;
		}
		// Look for the blank line
		http_rx_body_start = strstr(http_rx_buf, "\r\n\r\n");
		// We want to point to the first character of the body, not the empty line
		http_rx_body_start += 4;
		// Calculate the max possible response length
		http_content_length = sizeof(http_rx_buf) - ((size_t) (&http_rx_buf[0] - http_rx_body_start));
		LOG_INF("Content length is: %d", http_content_length);
		return response;
}

/* connects a socket to an HTTP addr:port/url
	This is taken from the zephyr http_client examples */
static int connect_socket(const char *hostname_str, int port, int *sock)
{
	int err;
	LOG_INF("connect_socket()");
	err = getaddrinfo(hostname_str, NULL, &addr_hints, &addr_res);
	if (err) {
		LOG_ERR("getaddrinfo(%s) failed, err %d\n", hostname_str, errno);
		return -1;
	}

	((struct sockaddr_in *)addr_res->ai_addr)->sin_port = htons(port);

	// Print out the resolved IP address
	if (addr_res->ai_addr->sa_family == AF_INET) {
        struct sockaddr_in *p = (struct sockaddr_in *)addr_res->ai_addr;
		net_addr_ntop(addr_res->ai_addr->sa_family, &p->sin_addr, &resolved_ip_addr[0], sizeof(resolved_ip_addr));
    } else if (addr_res->ai_addr->sa_family == AF_INET6) {
        struct sockaddr_in6 *p = (struct sockaddr_in6 *)addr_res->ai_addr;
		net_addr_ntop(addr_res->ai_addr->sa_family, &p->sin6_addr, &resolved_ip_addr[0], sizeof(resolved_ip_addr));
    }
	net_addr_ntop(addr_res->ai_family, addr_res->ai_addr, &resolved_ip_addr[0], sizeof(resolved_ip_addr));
	LOG_INF("Resolved target hostname to %s", resolved_ip_addr);

	*sock = socket(addr_res->ai_family, SOCK_STREAM, IPPROTO_TCP);
	if (*sock < 0)
	{
		LOG_ERR("Failed to create HTTP socket: %d", -errno);
		return -2;
	}

	err = connect(*sock, addr_res->ai_addr, sizeof(struct sockaddr_in));
	if (err < 0) {
		LOG_ERR("Cannot connect to remote: %d", -errno);
		return -errno;
	}
	LOG_INF("Connected to %s", hostname_str);
	return err;
}

static void web_poll() {
	int err = 0;
	while (true) {
		k_sleep(K_SECONDS(2));
		if (http_connected) {
			k_sem_take(&http_request_sem, K_FOREVER);
			err = get_request("/state.txt");
			if (err <= 0) {
				LOG_ERR("Empty or negative response code %d", err);
			} 
			else {
				cJSON *parsed_json = cJSON_ParseWithLength(http_rx_body_start, http_content_length);
				if (parsed_json == NULL)
				{
					LOG_ERR("Failed to parse light state from HTTP response!\nRESPONSE START\n%s\nRESPONSE END", http_rx_body_start);
					const char *error_ptr = cJSON_GetErrorPtr();
					if (error_ptr != NULL)
					{
						LOG_ERR("Error before: %s\n", error_ptr);
					}
				}
				else {
					const cJSON* light1_json = cJSON_GetObjectItemCaseSensitive(parsed_json, "light1");
					if (cJSON_IsString(light1_json) && (light1_json->valuestring != NULL))
					{
						struct ae_event* light_event = new_ae_event();
						light_event->cmd = AE_EVENT_LIGHT_CMD;
						light_event->target_light = AE_LIGHT1;
						light_event->new_light_state = string_to_light_state(light1_json->valuestring, 10);
						APP_EVENT_SUBMIT(light_event);
					}
					else {
						LOG_ERR("Failed to parse \"light1\" JSON field!");
					}
				}

				cJSON_Delete(parsed_json);
			}
			k_sem_give(&http_request_sem);
		}
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	LOG_INF("event");
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			LOG_INF("HTTP module setup");
			cJSON_InitHooks(&cjson_mem_hooks);
		}

		return false;
	}

	if (is_lte_event(aeh)) {
		const struct lte_event *event = cast_lte_event(aeh);
		if (event->conn_state == LTE_CONNECTED) {
			LOG_INF("Got LTE_CONNECTED");
			memset(&resolved_ip_addr[0], 0, INET6_ADDRSTRLEN);
			int err = connect_socket(ENDPOINT_HOSTNAME, ENDPOINT_PORT, &http_socket);
			if (err < 0) {
				LOG_ERR("connect_socket() failed!");
				return false;
			}
			else {
				LOG_INF("HTTP CONNECTED");
				http_connected = true;
			}
        }
		else if (event->conn_state == LTE_DISCONNECTED) {
			LOG_INF("Got LTE_DISCONNECTED");
			if (http_connected) {
				close(http_socket);
				http_connected = false;
				LOG_INF("HTTP DISCONNECTED");
			}
        }

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
static int post_request(char* url, char* payload[HTTP_RX_BUF_SIZE]) {
		// !!! WARNING !!!
		//    Make sure to call k_sem_take(&http_request_sem, K_FOREVER) before calling this function, 
		//		and then call k_sem_give(&http_request_sem) when you are done with the http_rx_buf !!!!! 
		//

		if (!http_connected) {
			LOG_WRN("Attempted to call push_request %s when http_connected is false!", url);
			return -1;
		}

		// Clear out the response buffer
		memset(&http_rx_buf, 0, sizeof(http_rx_buf));
		// Create a new request
		struct http_request req;
		memset(&req, 0, sizeof(req));
		req.method = HTTP_POST;
		req.url = url;
		req.host = ENDPOINT_HOSTNAME;
		req.protocol = "HTTP/1.1";
		req.response = response_cb;
		req.payload = payload;
		req.payload_len = sizeof(payload);
		req.recv_buf = &http_rx_buf[0];
		req.recv_buf_len = sizeof(http_rx_buf);

		int retry_count = 0;
		int response = 0;
		bool ok = false;
		while (retry_count < 3) {
			response = http_client_req(http_socket, &req, HTTP_REQUEST_TIMEOUT, NULL);
			if (response < 0) {
				LOG_ERR("http_client_req returned %d !", response);
				if (response == -128) {
					http_connected = false;
					int reconnect_response = connect_socket(ENDPOINT_HOSTNAME, ENDPOINT_PORT, &http_socket);
					if (reconnect_response < 0) {
						LOG_ERR("connect_socket() failed!");
						return -2;
					}
					else {
						LOG_INF("HTTP CONNECTED");
						http_connected = true;
					}
				}
				retry_count++;
				continue;
			}
			else {
				ok = true;
				break;
			}
		}
		if (!ok) {
			LOG_ERR("Retried %d times, quitting request!", retry_count);
			return -1;
		}
		return response;
}





APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, lte_event);

K_THREAD_DEFINE(web_poll_thread, 5120, web_poll, NULL, NULL, NULL, 10, 0, 0);