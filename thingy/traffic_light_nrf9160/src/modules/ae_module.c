/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>

#define MODULE traffic_light_ae
#include "events/module_state_event.h"
#include "events/uart_data_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

// AE state variables
bool ble_connected = false;
bool ble_scanning = false;
int light1_state = 0; // 0 = unknown, 1 = off, 2 = red, 3 = yellow, 4 = green
int light2_state = 0; // 0 = unknown, 1 = off, 2 = red, 3 = yellow, 4 = green

// nRF52840 Message parsing variables
#define BACKEND_PARSE_BUFFER_SIZE 100
char backend_parse_buf[BACKEND_PARSE_BUFFER_SIZE];
bool backend_started = false;
size_t backend_parse_buf_idx = 0; // the location in the backend_parse_buff that we are writing to


extern int uart_tx_enqueue(uint8_t *data, size_t data_len, uint8_t dev_idx); // from uart_handler.c
void send_command(const char* cmd) {
	LOG_INF("sending: %s", cmd);
	// Device index of 1 is to send to nRF52840
	uart_tx_enqueue((uint8_t*) cmd, strnlen(cmd, 30), 1);
}

void parse_nrf52840_char(char c) {
	if (backend_started && c != ';') {
		// Message has been started, but we are not at the end
		backend_parse_buf[backend_parse_buf_idx] = c;
		backend_parse_buf_idx++;
	}
	else if (!backend_started && c == '!') {
		// Start of message from nRF52840
		backend_parse_buf_idx = 0;
		memset(backend_parse_buf, 0, BACKEND_PARSE_BUFFER_SIZE);
		backend_started = true;
	}
	else if (backend_started && c == ';') {
		// Found end of the message
		if (strncmp(backend_parse_buf, "C", BACKEND_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got BLE_CONNECTED from nRF52840!");
			ble_connected = true;
		}
		else if (strncmp(backend_parse_buf, "D", BACKEND_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got BLE_DISCONNECTED from nRF52840!");
			ble_connected = false;
		}
		else if (strncmp(backend_parse_buf, "SCAN_START", BACKEND_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got BLE_SCAN_STARTED from nRF52840!");
			ble_scanning = true;
		}
		else if (strncmp(backend_parse_buf, "SCAN_STOP", BACKEND_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got BLE_SCAN_STOPPED from nRF52840!");
			ble_scanning = false;
		}
		else {
			LOG_ERR("Failed to parse message from nRF52840! %s", backend_parse_buf);
		}
		backend_started = false;
	}

	if (backend_parse_buf_idx >= BACKEND_PARSE_BUFFER_SIZE) {
		LOG_WRN("nRF52840 parse buf overrun!");
		backend_started = false;
	}
}

void ble_connection_management_thread() {
	while(true) {
		k_sleep(K_MSEC(500));
		if (!ble_connected && !ble_scanning) {
			send_command("!start_scan;");
		}
	}
}

void light_state_thread() {
	while(true) {
		// Only send commands if we are connected to the ESP32
		k_sleep(K_SECONDS(1));
		if (ble_connected) {
			// Simple color cycle
			if(light1_state == 2) {
				light1_state = 3;
				send_command("!yellow1;");
			}
			else if (light1_state == 3) {
				light1_state = 4;
				send_command("!green1;");
			}
			else if (light1_state == 4) {
				light1_state = 2;
				send_command("!red1;");
			}
			else {
				light1_state = 2;
				send_command("!red1;");
			}
		}
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{

	if (is_uart_data_event(aeh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(aeh);
		if (event->dev_idx == 1) {
		LOG_INF("Got data from 52");
            // This is coming from the nRF52840
            for (size_t i = 0; i < event->len; i++) {
				parse_nrf52840_char(event->buf[i]);
			}
        }

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			send_command("!start_scan;");

		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, uart_data_event);

K_THREAD_DEFINE(ble_conn_mgr, 4096, ble_connection_management_thread, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(light_mgr, 4096, light_state_thread, NULL, NULL, NULL, 7, 0, 0);