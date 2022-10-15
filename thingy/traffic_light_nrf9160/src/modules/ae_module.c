/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/pm/device.h>
#include <string.h>

#define MODULE traffic_light_ae
#include <caf/events/module_state_event.h>
#include "events/lte_event.h"
#include "events/ble_event.h"
#include "events/led_state_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

// AE state variables
bool lte_connected = false;
bool ble_connected = false;
bool ble_scanning = false;
int light1_state = 0; // 0 = unknown, 1 = off, 2 = red, 3 = yellow, 4 = green
int light2_state = 0; // 0 = unknown, 1 = off, 2 = red, 3 = yellow, 4 = green

extern int uart_tx_enqueue(uint8_t *data, size_t data_len, uint8_t dev_idx); // from uart_handler.c
void send_command(const char* cmd) {
	//LOG_INF("sending: %s", cmd);
	// Device index of 1 is to send to nRF52840
	uart_tx_enqueue((uint8_t*) cmd, strnlen(cmd, 30), 1);
}

void ble_connection_management_thread() {
	while(true) {
		k_sleep(K_MSEC(500));
		if (!lte_connected) {
			// LTE is not connected
			// Don't do anything with BLE until we have LTE
			continue;
		}
		else if (!ble_connected) {
			// LTE is connected, but BLE is not
			if (!ble_scanning) {
				send_command("!start_scan;");
			}
		}
	}
}

void light_state_thread() {
	while(true) {
		// Only send commands if we are connected to the ESP32
		k_sleep(K_SECONDS(1));
		if (!lte_connected && ble_connected) {
			light1_state = 3;
			send_command("!red1;");
			continue;
			// Keep the light red until LTE connection is re-established
		}
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

void set_yellow_led() {
	struct led_state_event* l = new_led_state_event();
	l->state = LED_STATE_YELLOW_BREATH;
	APP_EVENT_SUBMIT(l);
}

void set_green_led() {
	struct led_state_event* l = new_led_state_event();
	l->state = LED_STATE_GREEN_BREATH;
	APP_EVENT_SUBMIT(l);
}

void set_blue_led() {
	struct led_state_event* l = new_led_state_event();
	l->state = LED_STATE_BLUE_BREATH;
	APP_EVENT_SUBMIT(l);
}

static bool app_event_handler(const struct app_event_header *aeh)
{

	if (is_lte_event(aeh)) {
		const struct lte_event *event =
			cast_lte_event(aeh);
		if (event->conn_state == LTE_CONNECTED) {
			lte_connected = true;
			LOG_INF("Got LTE_CONNECTED");
			if (ble_connected) {
				set_green_led();
			}
        }
		else if (event->conn_state == LTE_DISCONNECTED) {
			lte_connected = false;
			LOG_INF("Got LTE_DISCONNECTED");
			set_yellow_led();
        }

		return false;
	}

	if (is_ble_event(aeh)) {
		const struct ble_event *event =
			cast_ble_event(aeh);
		if (event->cmd == BLE_CONNECTED) {
			ble_connected = true;
			if (lte_connected) {
				//set_green_led();
			}
        }
		else if (event->cmd == BLE_DISCONNECTED) {
			ble_connected = false;
			if (lte_connected) {
				//set_blue_led();
			}
        }
		else if (event->cmd == BLE_SCAN_STARTED) {
			ble_scanning = true;
        }
		else if (event->cmd == BLE_SCAN_STOPPED) {
			ble_scanning = false;
        }

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			send_command("!start_scan;");
			//set_yellow_led();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_event);
APP_EVENT_SUBSCRIBE(MODULE, lte_event);

// Giving these threads positive priorities means that they are pre-emptible (ie. other threads with positive priorities will run first)
K_THREAD_DEFINE(ble_conn_mgr, 2048, ble_connection_management_thread, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(light_mgr, 2048, light_state_thread, NULL, NULL, NULL, 10, 0, 0);