/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <zephyr/settings/settings.h>

#define MODULE nus_handler
#include "events/module_state_event.h"
#include "events/ae_command_event.h"
#include "events/ble_data_event.h"
#include "events/ble_ctrl_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

/*
    This module handles scanning, connecting, disconnecting, and sending data to the ESP32s over BLE with Nordic UART Service
*/

#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define NUS_WRITE_TIMEOUT K_MSEC(150)

K_SEM_DEFINE(nus_write_sem,1,1);

static struct bt_conn *default_conn;
static struct bt_nus_client nus_client;
bool ble_connected = false;
bool ble_scanning = false;

static void ble_data_sent(struct bt_nus_client *nus, uint8_t err,
					const uint8_t *const data, uint16_t len)
{
	ARG_UNUSED(nus);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	k_sem_give(&nus_write_sem);

	if (err) {
		LOG_WRN("ATT error code: 0x%02X", err);
	}
}

static uint8_t ble_data_received(struct bt_nus_client *nus,
						const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(nus);
	ARG_UNUSED(data);
    ARG_UNUSED(len);

	return BT_GATT_ITER_CONTINUE;
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_nus_client *nus = context;
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;
	LOG_INF("Beginning GATT service discovery...");
	if (conn != default_conn) {
		LOG_ERR("gett_discover, conn != default_conn !!");
		k_sleep(K_SECONDS(2));
		return;
	}
	
	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &discovery_cb,
			       &nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		LOG_INF("MTU exchange done");
	} else {
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("Failed to connect to %s (%d)", addr, conn_err);

		if (default_conn == conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err) {
				LOG_ERR("Scanning failed to start (err %d)",
					err);
			}
		}

		return;
	}

	LOG_INF("Connected: %s", addr);

	static struct bt_gatt_exchange_params exchange_params;

	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_WRN("MTU exchange failed (err %d)", err);
	}

	gatt_discover(conn);

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
	ble_scanning = false;

    // Fire off a CONNECTED event
	struct ble_ctrl_event *event;
    event = new_ble_ctrl_event();
    event->cmd = BLE_CTRL_CONNECTED;
    APP_EVENT_SUBMIT(event);
	ble_connected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", addr, reason);

	if (default_conn != conn) {
		LOG_ERR("disconnected() default_conn != conn");
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

    // Fire off a DISCONNECTED event
	struct ble_ctrl_event *event;
    event = new_ble_ctrl_event();
    event->cmd = BLE_CTRL_DISCONNECTED;
    APP_EVENT_SUBMIT(event);
	ble_connected = false;
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", addr,
			level, err);
	}

	gatt_discover(conn);
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d",
		addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_ERR("scan connecting error!");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    
	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d", addr, reason);
}


BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static int nus_client_init(void)
{
	int err = 0;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent
		}
	};

	err = bt_nus_client_init(&nus_client, &init);
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("NUS Client module initialized");
	return err;
}

static int scan_init(void)
{
	int err = 0;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_NUS_SERVICE);
	if (err) {
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}

static void nus_module_startup() {
    int err = 0;

    LOG_INF("Beginning BT call back setup...\n");

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks.");
		k_sleep(K_SECONDS(3));
		return;
	}
	
	LOG_INF("Did auth cb register, doing auth info cb...");
	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization info callbacks.\n");
		k_sleep(K_SECONDS(3));
		return;
	}

	LOG_INF("BT callbacks registered, attempting BT enable....\n");

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		k_sleep(K_SECONDS(2));
		return;
	}
	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
		LOG_INF("Called settings_load()\n");
	}

    err = scan_init();
    if(err) {
        LOG_ERR("scan_init() err: %d", err);
    }
    err = nus_client_init();
    if(err) {
        LOG_ERR("nus_client_init() err: %d", err);
    }
}

void send_ble_command(char* s) {
	// We must wait for the string to be sent over BLE, otherwise we risk invalidating the memory
	int err = k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT);
	if (err) {
		LOG_ERR("NUS send timeout");
	}

	err = bt_nus_client_send(&nus_client, s, strlen(s));
	if (err) {
		LOG_ERR("Failed to send data over BLE connection (err %d)", err);
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	int err;

    if (is_ae_command_event(aeh)) {
		const struct ae_command_event *event =
			cast_ae_command_event(aeh);
        switch (event->cmd) {
            case AE_CMD_START_SCAN:
				if (!ble_scanning) {
					err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
					if (err) {
						LOG_ERR("Scanning failed to start (err %d)", err);
					}
					ble_scanning = true;
				}
				// Fire off a SCANNING_STARTED event so that the AE knows we are scanning
				struct ble_ctrl_event *event2;
				event2 = new_ble_ctrl_event();
				event2->cmd = BLE_CTRL_SCAN_STARTED;
				APP_EVENT_SUBMIT(event2);
            break;
            case AE_CMD_STOP_SCAN:
                if (ble_scanning) {
					err = bt_scan_stop();
					if ((!err) && (err != -EALREADY)) {
						LOG_ERR("Stop LE scan failed (err %d)", err);
					}
				}
				ble_scanning = false;
				// Fire off a SCANNING_STOPPED event so that the AE knows we are not scanning
				struct ble_ctrl_event *event3;
				event3 = new_ble_ctrl_event();
				event3->cmd = BLE_CTRL_SCAN_STOPPED;
				APP_EVENT_SUBMIT(event3);
            break;
            case AE_CMD_SET_STATE1:
                //LOG_INF("AE CMD SET STATE 1");
                switch(event->light_state) {
					case LIGHT_OFF:
					send_ble_command("off1;");
					break;
					case LIGHT_RED:
					send_ble_command("red1;");
					break;
					case LIGHT_YELLOW:
					send_ble_command("yellow1;");
					break;
					case LIGHT_GREEN:
					send_ble_command("green1;");
					break;
					default:
					LOG_ERR("Unabled light state %d", event->light_state);
					break;
				}
            break;
            case AE_CMD_SET_STATE2:
                //LOG_INF("AE CMD SET STATE2");
                switch(event->light_state) {
					case LIGHT_OFF:
					send_ble_command("off2;");
					break;
					case LIGHT_RED:
					send_ble_command("red2;");
					break;
					case LIGHT_YELLOW:
					send_ble_command("yellow2;");
					break;
					case LIGHT_GREEN:
					send_ble_command("green2;");
					break;
					default:
					LOG_ERR("Unabled light state %d", event->light_state);
					break;
				}
            break;
            default:
                LOG_WRN("AE CMD type not handled! %d", event->cmd);
            break;
        }

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
            nus_module_startup();
		}

		return false;
	}

    if (is_ble_ctrl_event(aeh)) {
        // return true so that the App Event Handler knows to deallocated the event memory
        return true;
    }

    if (is_ble_data_event(aeh)) {
        // return true so that the App Event Handler knows to deallocated the event memory
        //const struct ble_data_event *event =
		//	cast_ble_data_event(aeh);

		/* All subscribers have gotten a chance to copy data at this point */
		//k_mem_slab_free(&ble_rx_slab, (void **) &event->buf);

		return false;
    }

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ae_command_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_ctrl_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_data_event);