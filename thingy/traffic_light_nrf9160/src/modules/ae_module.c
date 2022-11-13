/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/pm/device.h>
#include <string.h>

#include "deployment_settings.h"
#include "onem2m.h"

#define MODULE traffic_light_ae
#include <caf/events/module_state_event.h>
#include "events/lte_event.h"
#include "events/ble_event.h"
#include "events/ae_event.h"
#include "events/led_state_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define POLL_THREAD_STACK_SIZE 32768
#define POLLING_CHANNEL_PRIORITY 5
K_THREAD_STACK_DEFINE(poll_thread_stack, POLL_THREAD_STACK_SIZE);
struct k_thread polling_thread;

// The external function uart_tx_enqueue is defined in uart_handler.c
extern int uart_tx_enqueue(uint8_t *data, size_t data_len, uint8_t dev_idx); 

// AE state variables
bool lte_connected = false;
bool ble_connected = false;
bool ble_scanning = false;
enum ae_light_states light1_state = AE_LIGHT_RED;
enum ae_light_states light2_state = AE_LIGHT_RED;
bool poll_thread_started = false;
struct k_sem polling_sem;

void register_ae();
void create_data_model();

void take_poll_sem() {
	k_sem_take(&polling_sem, K_FOREVER);
}

void give_poll_sem() {
	k_sem_give(&polling_sem);
}

void send_command(const char* cmd) {
	// Device index of 1 is to send to nRF52840
	uart_tx_enqueue((uint8_t*) cmd, strlen(cmd), 1);
}

void push_flex_container() {
	char l1_state_string[10];
	char l2_state_string[10];
	memset(l1_state_string,0,10);
	memset(l2_state_string,0,10);
	light_state_to_string(light1_state, l1_state_string);
	light_state_to_string(light2_state, l2_state_string);

	char ble_string[20];
	memset(ble_string, 0, 20);
	if (ble_connected) {
		strcpy(ble_string, "connected");
	}
	else {
		strcpy(ble_string, "connected");
	}

	updateFlexContainer(l1_state_string, l2_state_string, ble_string);
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

void set_red_led() {
	struct led_state_event* l = new_led_state_event();
	l->state = LED_STATE_RED_SOLID;
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

void register_ae() {
	if (!discoverACP()) {
		createACP();
	}

	if (!discoverAE()) {
		createAE();
		createPCH();
	}
	else if (!discoverPCH()) {
		createPCH();
	}
}

void create_data_model() {
	if(!discoverFlexContainer()) {
		createFlexContainer();
		createSUB();
	}
	else if (!discoverSUB()) {
		createSUB();
	}
}

void do_poll() {
	while(true) {
		LOG_INF("poll loop");
		take_poll_sem();
		LOG_INF("take poll sem");
		onem2m_performPoll();
		LOG_INF("done poll");
		struct ae_event* a = new_ae_event();
		a->cmd = AE_EVENT_POLL;
		APP_EVENT_SUBMIT(a);
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if(is_ae_event(aeh)) {
		const struct ae_event *event = cast_ae_event(aeh);
		if (event->cmd == AE_EVENT_LIGHT_CMD) {
			light1_state = event->new_light1_state;
			light2_state = event->new_light2_state;
			update_light_states();
		}
		else if (event->cmd == AE_EVENT_POLL) {
			if (!poll_thread_started) {
				poll_thread_started = true;
				k_thread_create(&polling_thread, poll_thread_stack,
                                 K_THREAD_STACK_SIZEOF(poll_thread_stack),
                                 do_poll,
                                 NULL, NULL, NULL,
                                 POLLING_CHANNEL_PRIORITY, 0, K_NO_WAIT);
			}
			else {
				give_poll_sem();
			}
		}
		else if (event->cmd == AE_EVENT_REGISTER) {
			register_ae();
			if (event->do_init_sequence) {
				// Trigger the AE_EVENT_CREATE_DATA_MODEL EVENT
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_CREATE_DATA_MODEL;
				a->do_init_sequence = true;
				APP_EVENT_SUBMIT(a);
			}
		}
		else if (event->cmd == AE_EVENT_CREATE_DATA_MODEL) {
			create_data_model();
			if (event->do_init_sequence) {
				push_flex_container();
				// Trigger the AE_EVENT_POLL EVENT
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_POLL;
				APP_EVENT_SUBMIT(a);
			}
		}
		return false;
	}

	if (is_lte_event(aeh)) {
		const struct lte_event *event = cast_lte_event(aeh);
		if (event->conn_state == LTE_CONNECTED) {
			lte_connected = true;
			LOG_INF("Got LTE_CONNECTED");
			struct ae_event* a = new_ae_event();
			a->cmd = AE_EVENT_REGISTER;
			a->do_init_sequence = true;
			APP_EVENT_SUBMIT(a);
			
			if (ble_connected) {
				set_green_led();
			}
        }
		else if (event->conn_state == LTE_DISCONNECTED) {
			lte_connected = false;
			LOG_INF("Got LTE_DISCONNECTED");
			set_red_led();
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
				push_flex_container();
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
				push_flex_container();
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
			ble_scanning = false;
			lte_connected = false;
			poll_thread_started = false;
			k_sem_init(&polling_sem, 1, 1);
			send_command("!start_scan" BLE_TARGET ";");
			set_red_led();
			init_oneM2M();
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