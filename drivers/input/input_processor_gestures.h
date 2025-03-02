/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <drivers/input_processor.h>

struct gesture_event_t {
    uint32_t last_touch_timestamp;
    uint16_t x, y, previous_x, previous_y;
    int velocity_x, velocity_y;
    bool absolute;
    struct input_event *raw_event_1;
    struct input_event *raw_event_2;
};

typedef struct gesture_data gesture_data;
typedef struct gesture_config_t gesture_config;

typedef int (handle_init_t)(const struct device *dev);
typedef int (handle_touch_t)(const struct device *dev, struct gesture_event_t *event);
typedef int (handle_touch_end_t)(const struct device *dev);

#include "touch_detection.h"
#include "tap_detection.h"
#include "circular_scroll.h"
#include "inertial_cursor.h"

struct gesture_data {
    const struct device *dev;
    
    // I would prefer these to be pointers, but then dereferencing
    // the embedded k_work_delayable in there doesn't work:
    // &gesture_data->touch_detection.touch_end_timeout_work crashes 
    // the firmware :/
    struct touch_detection_data touch_detection;
    struct tap_detection_data tap_detection;
    struct circular_scroll_data circular_scroll;
    struct inertial_cursor_data inertial_cursor;
};

struct gesture_config {
    handle_touch_t *handle_touch_start;
    handle_touch_t *handle_touch_continue;
    handle_touch_end_t *handle_touch_end;
    
    struct touch_detection_config touch_detection;
    struct tap_detection_config tap_detection;
    struct circular_scroll_config circular_scroll;
    struct inertial_cursor_config inertial_cursor;
};