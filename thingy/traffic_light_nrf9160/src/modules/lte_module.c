/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/types.h>

#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include "events/lte_event.h"

#define MODULE lte_module
#include "events/module_state_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

K_SEM_DEFINE(lte_connected_sem, 0, 1);
static void lte_handler(const struct lte_lc_evt *const evt)
{
	LOG_INF("lte_handler called");
     switch (evt->type) {
     case LTE_LC_EVT_NW_REG_STATUS:
	 		LOG_INF("REG_STATUS");
             if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
             (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
                     break;
             }

             LOG_INF("Connected to: %s network\n",
             evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "home" : "roaming");

             k_sem_give(&lte_connected_sem);
             break;
     case LTE_LC_EVT_PSM_UPDATE:
     case LTE_LC_EVT_EDRX_UPDATE:
     case LTE_LC_EVT_RRC_UPDATE:
     case LTE_LC_EVT_CELL_UPDATE:
     case LTE_LC_EVT_LTE_MODE_UPDATE:
     case LTE_LC_EVT_TAU_PRE_WARNING:
     case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
     case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
     case LTE_LC_EVT_MODEM_SLEEP_EXIT:
     case LTE_LC_EVT_MODEM_SLEEP_ENTER:
             /* Callback events carrying LTE link data */
             break;
     default:
             break;
     }
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);
		LOG_INF("Got module state event...");
		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err = 0;
			k_sleep(K_SECONDS(5));
			LOG_INF("Disabling PSM and eDRX");
			err = lte_lc_psm_req(false);
			if (err) {
				LOG_ERR("lc psm req err: %d", err);
				return false;
			}
			err = lte_lc_edrx_req(false);
			if (err) {
				LOG_ERR("lc edrx req err: %d", err);
				return false;
			}
			LOG_INF("calling init and connect async");
			err = lte_lc_init_and_connect_async(lte_handler);
			if (err) {
					LOG_ERR("lte_lc_init_and_connect_async, error: %d\n", err);
					return false;
			}
			LOG_INF("Taking sem");
			k_sem_take(&lte_connected_sem, K_FOREVER);
			LOG_INF("got semaphore");
			struct lte_event* conn_event;
			conn_event = new_lte_event();
			conn_event->conn_state = LTE_CONNECTED;
			APP_EVENT_SUBMIT(conn_event);
		}

		return false;
	}

	if (is_lte_event(aeh)) {
		// Return true to let the application event manager know that it can deallocate it
		LOG_INF("Got lte event");
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, lte_event);