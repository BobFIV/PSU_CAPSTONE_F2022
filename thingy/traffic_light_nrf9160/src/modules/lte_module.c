/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdio.h>
#include <app_event_manager.h>
#include <math.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/pdn.h>

#define MODULE modem_module

#include "modules_common.h"
#include "events/module_state_event.h"
#include "events/modem_module_event.h"
#include "events/lte_event.h"

#ifdef CONFIG_LWM2M_CARRIER
#include <lwm2m_carrier.h>
#endif /* CONFIG_LWM2M_CARRIER */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
		"The Modem module does not support this configuration");

#ifdef CONFIG_APP_REQUEST_NEIGHBOR_CELLS_DATA
BUILD_ASSERT(CONFIG_AT_MONITOR_HEAP_SIZE >= 1024,
	    "CONFIG_AT_MONITOR_HEAP_SIZE must be >= 1024 to fit neighbor cell measurements "
	    "and other notifications at the same time");
#endif


struct modem_msg_data {
	union {
		struct modem_module_event modem;
	} module;
};

/* Modem module super states. */
static enum state_type {
	/* Initialization state where all libraries that the module depends
	 * on need to be initialized before you can enter any other state.
	 */
	STATE_INIT,
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_SHUTDOWN,
} state;

/* Enumerator that specifies the data type that is sampled. */
enum sample_type {
	NEIGHBOR_CELL,
	MODEM_DYNAMIC,
	MODEM_STATIC,
	BATTERY_VOLTAGE
};

/* Struct that holds data from the modem information module. */
static struct modem_param_info modem_param;

/* Value that holds the latest RSRP value. */
static int16_t rsrp_value_latest;

/* Value that holds the latest LTE network mode. */
static enum lte_lc_lte_mode nw_mode_latest;

const k_tid_t module_thread;

/* Modem module message queue. */
#define MODEM_QUEUE_ENTRY_COUNT		10
#define MODEM_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_modem, sizeof(struct modem_msg_data),
	      MODEM_QUEUE_ENTRY_COUNT, MODEM_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "modem",
	.msg_q = &msgq_modem,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void send_cell_update(uint32_t cell_id, uint32_t tac);
static void send_neighbor_cell_update(struct lte_lc_cells_info *cell_info);
static void send_psm_update(int tau, int active_time);
static void send_edrx_update(float edrx, float ptw);
static inline int adjust_rsrp(int input, enum sample_type type);
static inline int adjust_rsrq(int input);
static int lte_connect(void);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_DISCONNECTED:
		return "STATE_DISCONNECTED";
	case STATE_CONNECTING:
		return "STATE_CONNECTING";
	case STATE_CONNECTED:
		return "STATE_CONNECTED";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	if (new_state == STATE_CONNECTED) {
		struct lte_event* conn_event = new_lte_event();
		conn_event->conn_state = LTE_CONNECTED;
		APP_EVENT_SUBMIT(conn_event);
	}
	else if (new_state == STATE_DISCONNECTED) {
		struct lte_event* disconn_event = new_lte_event();
		disconn_event->conn_state = LTE_DISCONNECTED;
		APP_EVENT_SUBMIT(disconn_event);
	}

	LOG_INF("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct modem_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_lte_event(aeh)) {
		return true;
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);

		msg.module.modem = *evt;
		enqueue_msg = true;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			LOG_INF("module state ready");
		}

		return false;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	return false;
}

static void lte_evt_handler(const struct lte_lc_evt *const evt)
{
	LOG_INF("lte_evt_handler()...");
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS: {
		if (evt->nw_reg_status == LTE_LC_NW_REG_UICC_FAIL) {
			LOG_ERR("No SIM card detected!");
			SEND_ERROR(modem, MODEM_EVT_ERROR, -ENOTSUP);
			break;
		}

		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_DBG("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming");
		}

		break;
	}
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		send_psm_update(evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %.2f, PTW: %.2f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_buf);
		}

		send_edrx_update(evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		send_cell_update(evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
		if (evt->cells_info.current_cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {
			LOG_DBG("Neighbor cell measurements received");
			send_neighbor_cell_update((struct lte_lc_cells_info *)&evt->cells_info);
		} else {
			LOG_DBG("Neighbor cell measurement was not successful");
			SEND_EVENT(modem, MODEM_EVT_NEIGHBOR_CELLS_DATA_NOT_READY);
		}
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		nw_mode_latest = evt->lte_mode;
		break;
	case LTE_LC_EVT_MODEM_EVENT:
		LOG_DBG("Modem domain event, type: %s",
			evt->modem_evt == LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE ?
				"Light search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_SEARCH_DONE ?
				"Search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP ?
				"Reset loop" :
			evt->modem_evt == LTE_LC_MODEM_EVT_BATTERY_LOW ?
				"Low battery" :
			evt->modem_evt == LTE_LC_MODEM_EVT_OVERHEATED ?
				"Modem is overheated" :
				"Unknown");

		/* If a reset loop happens in the field, it should not be necessary
		 * to perform any action. The modem will try to re-attach to the LTE network after
		 * the 30-minute block.
		 */
		if (evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP) {
			LOG_WRN("The modem has detected a reset loop. LTE network attach is now "
				"restricted for the next 30 minutes. Power-cycle the device to "
				"circumvent this restriction. For more information see the "
				"nRF91 AT Commands - Command Reference Guide v2.0 - chpt. 5.36");
		}
		break;
	default:
		break;
	}
}

/* Handler that notifies the application of events related to the default PDN context, CID 0. */
void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	ARG_UNUSED(cid);

	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		LOG_WRN("Event: PDP context %d, %s", cid, pdn_esm_strerror(reason));
		break;
	case PDN_EVENT_ACTIVATED:
		LOG_DBG("PDN_EVENT_ACTIVATED");
		{ SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTED); }
		
		break;
	case PDN_EVENT_DEACTIVATED:
		LOG_DBG("PDN_EVENT_DEACTIVATED");
		{ SEND_EVENT(modem, MODEM_EVT_LTE_DISCONNECTED); }
		
		break;
	case PDN_EVENT_IPV6_UP:
		LOG_DBG("PDN_EVENT_IPV6_UP");
		break;
	case PDN_EVENT_IPV6_DOWN:
		LOG_DBG("PDN_EVENT_IPV6_DOWN");
		break;
	default:
		LOG_WRN("Unexpected PDN event!");
		break;
	}
}

static void modem_rsrp_handler(char rsrp_value)
{
	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97. RSRP is converted to dBm per "nRF91 AT Commands" v1.7.
	 */

	if (rsrp_value > 97) {
		return;
	}

	/* Set temporary variable to hold RSRP value. RSRP callbacks and other
	 * data from the modem info module are retrieved separately.
	 * This temporarily saves the latest value which are sent to
	 * the Data module upon a modem data request.
	 */
	rsrp_value_latest = adjust_rsrp(rsrp_value, MODEM_DYNAMIC);

	LOG_DBG("Incoming RSRP status message, RSRP value is %d",
		rsrp_value_latest);
}

/* Static module functions. */
static void send_cell_update(uint32_t cell_id, uint32_t tac)
{
	struct modem_module_event *evt = new_modem_module_event();

	evt->type = MODEM_EVT_LTE_CELL_UPDATE;
	evt->data.cell.cell_id = cell_id;
	evt->data.cell.tac = tac;

	APP_EVENT_SUBMIT(evt);
}

static void send_psm_update(int tau, int active_time)
{
	struct modem_module_event *evt = new_modem_module_event();

	evt->type = MODEM_EVT_LTE_PSM_UPDATE;
	evt->data.psm.tau = tau;
	evt->data.psm.active_time = active_time;

	APP_EVENT_SUBMIT(evt);
}

static void send_edrx_update(float edrx, float ptw)
{
	struct modem_module_event *evt = new_modem_module_event();

	evt->type = MODEM_EVT_LTE_EDRX_UPDATE;
	evt->data.edrx.edrx = edrx;
	evt->data.edrx.ptw = ptw;

	APP_EVENT_SUBMIT(evt);
}

static inline int adjust_rsrp(int input, enum sample_type type)
{
	switch (type) {
	case NEIGHBOR_CELL:
		if (IS_ENABLED(CONFIG_MODEM_NEIGHBOR_CELLS_DATA_CONVERT_RSRP_TO_DBM)) {
			return RSRP_IDX_TO_DBM(input);
		}
		break;
	case MODEM_DYNAMIC:
		if (IS_ENABLED(CONFIG_MODEM_DYNAMIC_DATA_CONVERT_RSRP_TO_DBM)) {
			return RSRP_IDX_TO_DBM(input);
		}
		break;
	default:
		LOG_WRN("Unknown sample type");
		break;
	}

	return input;
}

static inline int adjust_rsrq(int input)
{
	if (IS_ENABLED(CONFIG_MODEM_NEIGHBOR_CELLS_DATA_CONVERT_RSRQ_TO_DB)) {
		return round(RSRQ_IDX_TO_DB(input));
	}

	return input;
}

static void send_neighbor_cell_update(struct lte_lc_cells_info *cell_info)
{
	struct modem_module_event *evt = new_modem_module_event();

	BUILD_ASSERT(sizeof(evt->data.neighbor_cells.cell_data) ==
		     sizeof(struct lte_lc_cells_info));
	BUILD_ASSERT(sizeof(evt->data.neighbor_cells.neighbor_cells) >=
		     sizeof(struct lte_lc_ncell) * CONFIG_LTE_NEIGHBOR_CELLS_MAX);

	memcpy(&evt->data.neighbor_cells.cell_data, cell_info, sizeof(struct lte_lc_cells_info));
	memcpy(&evt->data.neighbor_cells.neighbor_cells, cell_info->neighbor_cells,
	       sizeof(struct lte_lc_ncell) * evt->data.neighbor_cells.cell_data.ncells_count);

	/* Convert RSRP to dBm and RSRQ to dB per "nRF91 AT Commands" v1.7. */
	evt->data.neighbor_cells.cell_data.current_cell.rsrp =
			adjust_rsrp(evt->data.neighbor_cells.cell_data.current_cell.rsrp,
				    NEIGHBOR_CELL);
	evt->data.neighbor_cells.cell_data.current_cell.rsrq =
			adjust_rsrq(evt->data.neighbor_cells.cell_data.current_cell.rsrq);

	for (size_t i = 0; i < evt->data.neighbor_cells.cell_data.ncells_count; i++) {
		evt->data.neighbor_cells.neighbor_cells[i].rsrp =
			adjust_rsrp(evt->data.neighbor_cells.neighbor_cells[i].rsrp,
				    NEIGHBOR_CELL);
		evt->data.neighbor_cells.neighbor_cells[i].rsrq =
			adjust_rsrq(evt->data.neighbor_cells.neighbor_cells[i].rsrq);
	}

	evt->type = MODEM_EVT_NEIGHBOR_CELLS_DATA_READY;
	evt->data.neighbor_cells.timestamp = k_uptime_get();

	APP_EVENT_SUBMIT(evt);
}

static int static_modem_data_get(void)
{
	int err;

	/* Request data from modem information module. */
	err = modem_info_params_get(&modem_param);
	if (err) {
		LOG_ERR("modem_info_params_get, error: %d", err);
		return err;
	}

	struct modem_module_event *modem_module_event = new_modem_module_event();

	strncpy(modem_module_event->data.modem_static.app_version,
		"4",
		sizeof(modem_module_event->data.modem_static.app_version) - 1);

	strncpy(modem_module_event->data.modem_static.board_version,
		modem_param.device.board,
		sizeof(modem_module_event->data.modem_static.board_version) - 1);

	strncpy(modem_module_event->data.modem_static.modem_fw,
		modem_param.device.modem_fw.value_string,
		sizeof(modem_module_event->data.modem_static.modem_fw) - 1);

	strncpy(modem_module_event->data.modem_static.iccid,
		modem_param.sim.iccid.value_string,
		sizeof(modem_module_event->data.modem_static.iccid) - 1);

	strncpy(modem_module_event->data.modem_static.imei,
		modem_param.device.imei.value_string,
		sizeof(modem_module_event->data.modem_static.imei) - 1);

	modem_module_event->data.modem_static.app_version
		[sizeof(modem_module_event->data.modem_static.app_version) - 1] = '\0';

	modem_module_event->data.modem_static.board_version
		[sizeof(modem_module_event->data.modem_static.board_version) - 1] = '\0';

	modem_module_event->data.modem_static.modem_fw
		[sizeof(modem_module_event->data.modem_static.modem_fw) - 1] = '\0';

	modem_module_event->data.modem_static.iccid
		[sizeof(modem_module_event->data.modem_static.iccid) - 1] = '\0';

	modem_module_event->data.modem_static.imei
		[sizeof(modem_module_event->data.modem_static.imei) - 1] = '\0';

	modem_module_event->data.modem_static.timestamp = k_uptime_get();
	modem_module_event->type = MODEM_EVT_MODEM_STATIC_DATA_READY;

	APP_EVENT_SUBMIT(modem_module_event);
	return 0;
}

static void populate_event_with_dynamic_modem_data(struct modem_module_event *event,
						   struct modem_param_info *param)
{
	/* If this flag is set all sampled parameter values will be included in the event regardless
	 * if they have changed or not.
	 */
	bool include = IS_ENABLED(CONFIG_MODEM_SEND_ALL_SAMPLED_DATA);

	/* Flag that checks if parameters has been added to the event. */
	bool params_added = false;

	/* Set all entries in the dynamic modem data structure to 0 to be sure that all 'fresh'
	 * flags become false by default. This is to avoid sending garbage or old data due to a flag
	 * being accidently set to true.
	 */
	memset(&event->data.modem_dynamic, 0, sizeof(struct modem_module_dynamic_modem_data));

	/* Structure that holds previous sampled dynamic modem data. By default, set all members of
	 * the structure to invalid values.
	 */
	static struct modem_module_dynamic_modem_data prev = {
		.rsrp = UINT8_MAX,
		.nw_mode = LTE_LC_LTE_MODE_NONE,
	};

	/* Compare the latest sampled parameters with the previous. If there has been a change we
	 * want to include the parameters in the event.
	 */
	if ((prev.rsrp != rsrp_value_latest) || include) {
		event->data.modem_dynamic.rsrp = rsrp_value_latest;
		prev.rsrp = rsrp_value_latest;

		event->data.modem_dynamic.rsrp_fresh = true;
		params_added = true;
	}

	if ((prev.band != param->network.current_band.value) || include) {
		event->data.modem_dynamic.band = param->network.current_band.value;
		prev.band = param->network.current_band.value;

		event->data.modem_dynamic.band_fresh = true;
		params_added = true;
	}

	if ((prev.nw_mode != nw_mode_latest) || include) {
		event->data.modem_dynamic.nw_mode = nw_mode_latest;
		prev.nw_mode = nw_mode_latest;

		event->data.modem_dynamic.nw_mode_fresh = true;
		params_added = true;
	}

	if ((strcmp(prev.apn, param->network.apn.value_string) != 0) || include) {
		strncpy(event->data.modem_dynamic.apn,
			modem_param.network.apn.value_string,
			sizeof(event->data.modem_dynamic.apn) - 1);

		strncpy(prev.apn,
			param->network.apn.value_string,
			sizeof(prev.apn) - 1);

		event->data.modem_dynamic.apn
			[sizeof(event->data.modem_dynamic.apn) - 1] = '\0';

		prev.apn[sizeof(prev.apn) - 1] = '\0';

		event->data.modem_dynamic.apn_fresh = true;
		params_added = true;
	}

	if ((strcmp(prev.ip_address, param->network.ip_address.value_string) != 0) || include) {
		strncpy(event->data.modem_dynamic.ip_address,
			modem_param.network.ip_address.value_string,
			sizeof(event->data.modem_dynamic.ip_address) - 1);

		strncpy(prev.ip_address,
			param->network.ip_address.value_string,
			sizeof(prev.ip_address) - 1);

		event->data.modem_dynamic.ip_address
			[sizeof(event->data.modem_dynamic.ip_address) - 1] = '\0';

		prev.ip_address[sizeof(prev.ip_address) - 1] = '\0';

		event->data.modem_dynamic.ip_address_fresh = true;
		params_added = true;
	}

	if ((prev.cell_id != param->network.cellid_dec) || include) {
		event->data.modem_dynamic.cell_id = param->network.cellid_dec;
		prev.cell_id = param->network.cellid_dec;

		event->data.modem_dynamic.cell_id_fresh = true;
		params_added = true;
	}

	if ((strcmp(prev.mccmnc, param->network.current_operator.value_string) != 0) || include) {
		strncpy(event->data.modem_dynamic.mccmnc,
			modem_param.network.current_operator.value_string,
			sizeof(event->data.modem_dynamic.mccmnc));

		strncpy(prev.mccmnc, param->network.current_operator.value_string,
			sizeof(prev.mccmnc));

		event->data.modem_dynamic.mccmnc
			[sizeof(event->data.modem_dynamic.mccmnc) - 1] = '\0';

		prev.mccmnc[sizeof(prev.mccmnc) - 1] = '\0';

		/* Provide MNC and MCC as separate values. */
		event->data.modem_dynamic.mcc = modem_param.network.mcc.value;
		event->data.modem_dynamic.mnc = modem_param.network.mnc.value;

		event->data.modem_dynamic.mccmnc_fresh = true;
		params_added = true;
	}

	if ((prev.area_code != modem_param.network.area_code.value) || include) {
		event->data.modem_dynamic.area_code = param->network.area_code.value;
		prev.area_code = param->network.area_code.value;

		event->data.modem_dynamic.area_code_fresh = true;
		params_added = true;
	}

	if (params_added) {
		event->type = MODEM_EVT_MODEM_DYNAMIC_DATA_READY;
		event->data.modem_dynamic.timestamp = k_uptime_get();
	} else {
		LOG_DBG("No dynamic modem parameters have changed from the last sample request.");
		event->type = MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY;
	}
}

static int dynamic_modem_data_get(void)
{
	int err;

	/* Request data from modem information module. */
	err = modem_info_params_get(&modem_param);
	if (err) {
		LOG_ERR("modem_info_params_get, error: %d", err);
		return err;
	}

	struct modem_module_event *modem_module_event = new_modem_module_event();

	populate_event_with_dynamic_modem_data(modem_module_event, &modem_param);

	APP_EVENT_SUBMIT(modem_module_event);
	return 0;
}

static int configure_low_power(void)
{
	int err;
	LOG_INF("configure_low_power");
	err = lte_lc_psm_req(false);
	if (err) {
		LOG_ERR("lte_lc_psm_req, error: %d", err);
		return err;
	}

	LOG_DBG("PSM disabled");

	return 0;
}

static int lte_connect(void)
{
	int err;
	LOG_INF("lte_connect executing...");
	err = lte_lc_connect_async(lte_evt_handler);
	if (err) {
		LOG_ERR("lte_lc_connect_async, error: %d", err);

		return err;
	}

	SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTING);

	return 0;
}

static int modem_data_init(void)
{
	int err;
	LOG_INF("modem_data_init()");
	err = modem_info_init();
	if (err) {
		LOG_INF("modem_info_init, error: %d", err);
		return err;
	}

	err = modem_info_params_init(&modem_param);
	if (err) {
		LOG_INF("modem_info_params_init, error: %d", err);
		return err;
	}

	err = modem_info_rsrp_register(modem_rsrp_handler);
	if (err) {
		LOG_INF("modem_info_rsrp_register, error: %d", err);
		return err;
	}

	return 0;
}

static int setup(void)
{
	int err;
	LOG_INF("setup()");
	err = lte_lc_init();
	if (err) {
		LOG_ERR("lte_lc_init, error: %d", err);
		return err;
	}

	/* Setup a callback for the default PDP context. */
	err = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (err) {
		LOG_ERR("pdn_default_ctx_cb_reg, error: %d", err);
		return err;
	}

	err = configure_low_power();
	if (err) {
		LOG_ERR("configure_low_power, error: %d", err);
		return err;
	}

	err = lte_lc_modem_events_enable();
	if (err) {
		LOG_ERR("lte_lc_modem_events_enable failed, error: %d", err);
		return err;
	}

	err = modem_data_init();
	if (err) {
		LOG_ERR("modem_data_init, error: %d", err);
		return err;
	}

	return 0;
}

/* Message handler for STATE_INIT */
static void on_state_init(struct modem_msg_data *msg)
{
	LOG_INF("on_state_init");
	if (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_INITIALIZED)) {
		int err;

		state_set(STATE_DISCONNECTED);

		err = setup();
		__ASSERT(err == 0, "Failed running setup()");
		SEND_EVENT(modem, MODEM_EVT_INITIALIZED);

		err = lte_connect();
		if (err) {
			LOG_ERR("Failed connecting to LTE, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
		
	}
}

/* Message handler for STATE_DISCONNECTED. */
static void on_state_disconnected(struct modem_msg_data *msg)
{
	LOG_INF("on_state_disconnected");
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) {
		state_set(STATE_CONNECTED);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTING)) {
		state_set(STATE_CONNECTING);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_EVENT_LTE_LINK_UP_REQUEST) || IS_EVENT(msg, modem, MODEM_EVT_INITIALIZED)) {
		int err;

		err = lte_connect();
		if (err) {
			LOG_ERR("Failed connecting to LTE, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}
}

/* Message handler for STATE_CONNECTING. */
static void on_state_connecting(struct modem_msg_data *msg)
{
	/*if ((IS_EVENT(msg, app, APP_EVT_LTE_DISCONNECT)) ||
	    (IS_EVENT(msg, cloud, CLOUD_EVT_LTE_DISCONNECT))) {
		int err;

		err = lte_lc_offline();
		if (err) {
			LOG_ERR("LTE disconnect failed, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}

		state_set(STATE_DISCONNECTED);
	}*/
	LOG_INF("on state connectiong");
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) {
		state_set(STATE_CONNECTED);
	}
}

/* Message handler for STATE_CONNECTED. */
static void on_state_connected(struct modem_msg_data *msg)
{
	LOG_INF("on state connected");
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_DISCONNECTED)) {
		state_set(STATE_DISCONNECTED);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_EVENT_LTE_LINK_DOWN_REQUEST)) {
		int err;

		err = lte_lc_offline();
		if (err) {
			LOG_ERR("LTE disconnect failed, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_EVENT_LTE_LINK_DOWN_REQUEST)) {
		int err;

		err = lte_lc_offline();
		if (err) {
			LOG_ERR("LTE disconnect failed, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}

		state_set(STATE_DISCONNECTED);
	}
}

/* Message handler for all states. */
static void on_all_states(struct modem_msg_data *msg)
{
/*
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err;

		if (IS_ENABLED(CONFIG_LWM2M_CARRIER)) {
			
			return;
		}

		err = lte_connect();
		if (err) {
			LOG_ERR("Failed connecting to LTE, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}
	}*/
}

static void module_thread_fn(void)
{
	int err;
	struct modem_msg_data msg = { 0 };
	LOG_INF("moduel thread begin");
	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(modem, MODEM_EVT_ERROR, err);
	}
	
	if (IS_ENABLED(CONFIG_LWM2M_CARRIER)) {
		state_set(STATE_INIT);
	} else {
		state_set(STATE_DISCONNECTED);
		SEND_EVENT(modem, MODEM_EVT_INITIALIZED);
		LOG_INF("moduel thread calling setup");
		err = setup();
		if (err) {
			LOG_ERR("Failed setting up the modem, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_INIT:
			on_state_init(&msg);
			break;
		case STATE_DISCONNECTED:
			on_state_disconnected(&msg);
			break;
		case STATE_CONNECTING:
			on_state_connecting(&msg);
			break;
		case STATE_CONNECTED:
			on_state_connected(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_WRN("Invalid state: %d", state);
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(modem_module_thread, 2048,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, lte_event);