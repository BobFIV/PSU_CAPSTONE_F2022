/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "events/ae_command_event.h"

static void log_ae_command_event(const struct app_event_header *aeh)
{
	const struct ae_command_event *event = cast_ae_command_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "command type:%d", event->cmd);
}

APP_EVENT_TYPE_DEFINE(ae_command_event,
		  log_ae_command_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_LOG_AE_CMD_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
