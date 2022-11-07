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
#include "events/ae_event.h"
#include "events/led_state_event.h"

#include "deployment_settings.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

// The external function uart_tx_enqueue is defined in uart_handler.c
extern int uart_tx_enqueue(uint8_t *data, size_t data_len, uint8_t dev_idx); 

// AE state variables
bool lte_connected = false;
bool ble_connected = false;
bool ble_scanning = false;
enum ae_light_states light1_state = AE_LIGHT_RED;
enum ae_light_states light2_state = AE_LIGHT_RED;

void send_command(const char* cmd) {
	// Device index of 1 is to send to nRF52840
	uart_tx_enqueue((uint8_t*) cmd, strnlen(cmd, 30), 1);
}

void ble_connection_management_thread() {
	while(true) {
		k_sleep(K_MSEC(1000));
		if (!lte_connected) {
			// LTE is not connected
			// Don't do anything with BLE until we have LTE
			continue;
		}
		else if (!ble_connected) {
			// LTE is connected, but BLE is not
			if (!ble_scanning) {
				send_command("!start_scan" BLE_TARGET ";");
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

void update_light_states() {
	switch (light1_state) {
		case AE_LIGHT_OFF:
		send_command("!off1;");
		break;
		case AE_LIGHT_RED:
		send_command("!red1;");
		break;
		case AE_LIGHT_YELLOW:
		send_command("!yellow1;");
		break;
		case AE_LIGHT_GREEN:
		send_command("!green1;");
		break;
		default:
		// Do nothing
		break;
	}
	switch (light2_state) {
		case AE_LIGHT_OFF:
		send_command("!off2;");
		break;
		case AE_LIGHT_RED:
		send_command("!red2;");
		break;
		case AE_LIGHT_YELLOW:
		send_command("!yellow2;");
		break;
		case AE_LIGHT_GREEN:
		send_command("!green2;");
		break;
		default:
		// Do nothing
		break;
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if(is_ae_event(aeh)) {
		const struct ae_event *event = cast_ae_event(aeh);
		if (event->cmd == AE_EVENT_LIGHT_CMD) {
			if (event->target_light == AE_LIGHT1) {
				light1_state = event->new_light_state;
			}
			else if (event->target_light == AE_LIGHT2) {
				light2_state = event->new_light_state;
			}

			if (ble_connected && lte_connected) {
				update_light_states();
			}
		}

		return false;
	}

	if (is_lte_event(aeh)) {
		const struct lte_event *event = cast_lte_event(aeh);
		if (event->conn_state == LTE_CONNECTED) {
			lte_connected = true;
			LOG_INF("Got LTE_CONNECTED");
			if (ble_connected) {
				set_green_led();
				update_light_states();
			}
        }
		else if (event->conn_state == LTE_DISCONNECTED) {
			lte_connected = false;
			LOG_INF("Got LTE_DISCONNECTED");
			set_yellow_led();
			// If we are paired with a traffic light, set it to RED until we re-establish our connection
			light1_state = AE_LIGHT_RED;
			light2_state = AE_LIGHT_RED;
			if (ble_connected) {
				update_light_states();
			}
        }
		return false;
	}

	if (is_ble_event(aeh)) {
		const struct ble_event *event = cast_ble_event(aeh);
		if (event->cmd == BLE_CONNECTED) {
			ble_connected = true;
			if (lte_connected) {
				set_green_led();
				update_light_states();
			}
			else {
				light1_state = AE_LIGHT_RED;
				light2_state = AE_LIGHT_RED;
				update_light_states();
			}
			send_command("!stop_scan;");
        }
		else if (event->cmd == BLE_DISCONNECTED) {
			ble_connected = false;
			if (lte_connected) {
				set_blue_led();
			}
			send_command("!start_scan" BLE_TARGET ";");
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
			send_command("!start_scan" BLE_TARGET ";");
			set_yellow_led();
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
APP_EVENT_SUBSCRIBE(MODULE, ae_event);

// Giving these threads positive priorities means that they are pre-emptible (ie. other threads with positive priorities will run first)
K_THREAD_DEFINE(ble_conn_mgr, 2048, ble_connection_management_thread, NULL, NULL, NULL, 7, 0, 0);