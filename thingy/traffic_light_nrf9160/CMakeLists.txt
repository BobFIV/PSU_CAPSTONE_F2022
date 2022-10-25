#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(traffic_light_ae)

target_sources(app PRIVATE
  src/main.c
  src/onem2m.c

  src/events/ble_event.c
  src/events/ae_event.c
  src/events/led_state_event.c
  src/events/lte_event.c
  src/events/modem_module_event.c
  src/events/peer_conn_event.c
  src/events/uart_data_event.c

  src/modules/ae_module.c
  src/modules/at_handler.c
  src/modules/http_module.c
  src/modules/lte_module.c
  src/modules/led_module.c
  src/modules/modules_common.c
  src/modules/nrf52840_parser.c
  src/modules/uart_handler.c
)

target_include_directories(app PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
