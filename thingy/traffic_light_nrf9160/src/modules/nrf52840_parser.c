/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>

#define MODULE nrf52840_parser
#include <caf/events/module_state_event.h>
#include "events/uart_data_event.h"
#include "events/ble_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

// nRF52840 Message parsing variables
#define BACKEND_PARSE_BUFFER_SIZE 100
char backend_parse_buf[BACKEND_PARSE_BUFFER_SIZE];
bool backend_started = false;
size_t backend_parse_buf_idx = 0; // the location in the backend_parse_buff that we are writing to

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
			struct ble_event* b = new_ble_event();
			b->cmd = BLE_CONNECTED;
			APP_EVENT_SUBMIT(b);
		}
		else if (strncmp(backend_parse_buf, "D", BACKEND_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got BLE_DISCONNECTED from nRF52840!");
			struct ble_event* b = new_ble_event();
			b->cmd = BLE_DISCONNECTED;
			APP_EVENT_SUBMIT(b);
		}
		else if (strncmp(backend_parse_buf, "SCAN_START", BACKEND_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got BLE_SCAN_STARTED from nRF52840!");
			struct ble_event* b = new_ble_event();
			b->cmd = BLE_SCAN_STARTED;
			APP_EVENT_SUBMIT(b);
		}
		else if (strncmp(backend_parse_buf, "SCAN_STOP", BACKEND_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got BLE_SCAN_STOPPED from nRF52840!");
			struct ble_event* b = new_ble_event();
			b->cmd = BLE_SCAN_STOPPED;
			APP_EVENT_SUBMIT(b);
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

static bool app_event_handler(const struct app_event_header *aeh)
{

	if (is_uart_data_event(aeh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(aeh);
		if (event->dev_idx == 1) {
            // This is coming from the nRF52840
            for (size_t i = 0; i < event->len; i++) {
				parse_nrf52840_char(event->buf[i]);
			}
        }

		return false;
	}

	if (is_ble_event(aeh)) {
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, uart_data_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_event);
