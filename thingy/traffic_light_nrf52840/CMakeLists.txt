#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_traffic_light_control)

target_sources(app PRIVATE
  src/main.c
  src/events/uart_data_event.c
  src/events/module_state_event.c
  src/events/cdc_data_event.c
  src/events/ble_data_event.c
  src/events/ble_ctrl_event.c
  src/events/peer_conn_event.c
)

target_include_directories(app PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
