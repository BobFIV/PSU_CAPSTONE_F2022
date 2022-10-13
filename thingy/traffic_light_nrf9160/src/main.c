/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/settings/settings.h>

#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>
#include <zephyr/console/console.h>

#include <app_event_manager.h>

#define MODULE main
#include "events/module_state_event.h"
#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

extern int uart_tx_enqueue(uint8_t *data, size_t data_len, uint8_t dev_idx);

void send_command(char* cmd) {
	LOG_INF("sending: %s", cmd);
	uart_tx_enqueue((uint8_t*) cmd, strnlen(cmd, 30), 0);
}

void main() {

	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized!");
		k_sleep(K_SECONDS(2));
		return;
	} 

	LOG_INF("Application Event Manager initialized.");
	module_set_state(MODULE_STATE_READY);
	// TODO: Have a while loop here.
	LOG_INF("Returned from module_set_state_ready");
	while(true) {
		k_sleep(K_SECONDS(1));
		LOG_INF("Boop from 9160!");
		k_sleep(K_SECONDS(1));
		send_command("!red1;");
		k_sleep(K_SECONDS(1));
		send_command("!yellow1;");
		k_sleep(K_SECONDS(1));
		send_command("!green1;");
	}
}
