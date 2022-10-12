/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/scan.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>

#define MODULE nus_handler
#include "events/module_state_event.h"
#include "events/nus_ctrl_event.h"
#include "events/nus_data_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, );

#define BLE_RX_BLOCK_SIZE (CONFIG_BT_L2CAP_TX_MTU - 3)
#define BLE_RX_BUF_COUNT 4
#define BLE_SLAB_ALIGNMENT 4

#define BLE_TX_BUF_SIZE (CONFIG_BRIDGE_BUF_SIZE * 2)

#define BLE_AD_IDX_FLAGS 0
#define BLE_AD_IDX_NAME 1

#define ATT_MIN_PAYLOAD 20 /* Minimum L2CAP MTU minus ATT header */

static void bt_send_work_handler(struct k_work *work);

K_MEM_SLAB_DEFINE(ble_rx_slab, BLE_RX_BLOCK_SIZE, BLE_RX_BUF_COUNT, BLE_SLAB_ALIGNMENT);
RING_BUF_DECLARE(ble_tx_ring_buf, BLE_TX_BUF_SIZE);

static K_SEM_DEFINE(ble_tx_sem, 0, 1);

static K_WORK_DEFINE(bt_send_work, bt_send_work_handler);

static struct bt_conn *current_conn;
static struct bt_gatt_exchange_params exchange_params;
static uint32_t nus_max_send_len;
static atomic_t ready;
static atomic_t active;

static char bt_device_name[CONFIG_BT_DEVICE_NAME_MAX + 1] = CONFIG_BT_DEVICE_NAME;

static struct bt_data ad[] = {
    [BLE_AD_IDX_FLAGS] = BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    [BLE_AD_IDX_NAME] = BT_DATA(BT_DATA_NAME_COMPLETE, bt_device_name, (sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static void exchange_func(struct bt_conn *conn, uint8_t err,
              struct bt_gatt_exchange_params *params)
{
    if (!err) {
        nus_max_send_len = bt_nus_get_mtu(conn);
    }
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (err) {
        LOG_WRN("Connection failed (err %u)", err);
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected %s", addr);

    current_conn = bt_conn_ref(conn);
    exchange_params.func = exchange_func;

    err = bt_gatt_exchange_mtu(current_conn, &exchange_params);
    if (err) {
        LOG_WRN("bt_gatt_exchange_mtu: %d", err);
    }

    ring_buf_reset(&ble_tx_ring_buf);

    struct peer_conn_event *event = new_peer_conn_event();

    event->peer_id = PEER_ID_BLE;
    event->dev_idx = 0;
    event->baudrate = 0; /* Don't care */
    event->conn_state = PEER_STATE_CONNECTED;
    APP_EVENT_SUBMIT(event);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected: %s (reason %u)", addr, reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    struct peer_conn_event *event = new_peer_conn_event();

    event->peer_id = PEER_ID_BLE;
    event->dev_idx = 0;
    event->baudrate = 0; /* Don't care */
    event->conn_state = PEER_STATE_DISCONNECTED;
    APP_EVENT_SUBMIT(event);
}

static void bt_send_work_handler(struct k_work *work)
{
    uint16_t len;
    uint8_t *buf;
    int err;
    bool notif_disabled = false;

    do {
        len = ring_buf_get_claim(&ble_tx_ring_buf, &buf, nus_max_send_len);

        err = bt_nus_send(current_conn, buf, len);
        if (err == -EINVAL) {
            notif_disabled = true;
            len = 0;
        } else if (err) {
            len = 0;
        }

        err = ring_buf_get_finish(&ble_tx_ring_buf, len);
        if (err) {
            LOG_ERR("ring_buf_get_finish: %d", err);
            break;
        }
    } while (len != 0 && !ring_buf_is_empty(&ble_tx_ring_buf));

    if (notif_disabled) {
        /* Peer has not enabled notifications: don't accumulate data */
        ring_buf_reset(&ble_tx_ring_buf);
    }
}

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
              uint16_t len)
{
    void *buf;
    uint16_t remainder;

    remainder = len;

    do {
        uint16_t copy_len;
        int err;

        err = k_mem_slab_alloc(&ble_rx_slab, &buf, K_NO_WAIT);
        if (err) {
            LOG_WRN("BLE RX overflow");
            break;
        }

        copy_len = remainder > BLE_RX_BLOCK_SIZE ?
            BLE_RX_BLOCK_SIZE : remainder;
        remainder -= copy_len;
        memcpy(buf, data, copy_len);

        struct ble_data_event *event = new_ble_data_event();

        event->buf = buf;
        event->len = copy_len;
        APP_EVENT_SUBMIT(event);
    } while (remainder);
}

static void bt_sent_cb(struct bt_conn *conn)
{
    if (ring_buf_is_empty(&ble_tx_ring_buf)) {
        return;
    }

    k_work_submit(&bt_send_work);
}

static void adv_start(void)
{
    int err;

    if (!atomic_get(&ready)) {
        /* Advertising will start when ready */
        return;
    }

    err = bt_le_adv_start(
        BT_LE_ADV_PARAM(
            BT_LE_ADV_OPT_CONNECTABLE,
            BT_GAP_ADV_SLOW_INT_MIN,
            BT_GAP_ADV_SLOW_INT_MAX,
            NULL),
        ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("bt_le_adv_start: %d", err);
    } else {
        module_set_state(MODULE_STATE_READY);
    }
}

static void adv_stop(void)
{
    int err;

    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("bt_le_adv_stop: %d", err);
    } else {
        module_set_state(MODULE_STATE_STANDBY);
    }
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

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};

static bool app_event_handler(const struct app_event_header *aeh)
{

    if (is_ble_data_event(aeh)) {
        const struct ble_data_event *event =
            cast_ble_data_event(aeh);

        /* All subscribers have gotten a chance to copy data at this point */
        k_mem_slab_free(&ble_rx_slab, (void **) &event->buf);

        return false;
    }

    if (is_ble_ctrl_event(aeh)) {
        const struct ble_ctrl_event *event =
            cast_ble_ctrl_event(aeh);

        switch (event->cmd) {
        case BLE_CTRL_ENABLE:
            if (!atomic_set(&active, true)) {
                adv_start();
            }
            break;
        case BLE_CTRL_DISABLE:
            if (atomic_set(&active, false)) {
                adv_stop();
            }
            break;
        case BLE_CTRL_NAME_UPDATE:
            name_update(event->param.name_update);
            break;
        default:
            /* Unhandled control message */
            __ASSERT_NO_MSG(false);
            break;
        }

        return false;
    }

    if (is_module_state_event(aeh)) {
        const struct module_state_event *event =
            cast_module_state_event(aeh);

        if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
            int err;

            atomic_set(&active, false);

            nus_max_send_len = ATT_MIN_PAYLOAD;

            BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
                    scan_connecting_error, scan_connecting);

            err = bt_enable(bt_ready);
            if (err) {
                LOG_ERR("bt_enable: %d", err);
                return false;
            }

            //bt_conn_cb_register(&conn_callbacks);
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
        }

        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_ctrl_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_data_event);
