/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

typedef struct gesture_data gesture_data;
typedef struct gesture_config_t gesture_config;

#include "tap_detection.h"

struct touch_detection_data {
    bool touching;
    struct k_work_delayable touch_end_timeout_work;
    uint32_t last_touch_timestamp;
    uint16_t previous_abs_x, previous_abs_y;
    uint16_t previous_rel_x, previous_rel_y;
    gesture_data *all;
};

struct touch_detection_config {
    const uint8_t wait_for_new_position_ms;
};

struct inertial_cursor_data {
    gesture_data *all;
};

struct inertial_cursor_config {
    const bool enabled;
    const uint8_t inertial_cursor_velocity_threshold_ms, inertial_cursor_decay_percent;
};

struct gesture_data {
    const struct device *dev;
    
    // I would prefer these to be pointers, but then dereferencing
    // the embedded k_work_delayable in there doesn't work:
    // &gesture_data->touch_detection.touch_end_timeout_work crashes 
    // the firmware :/
    struct touch_detection_data touch_detection;
    struct tap_detection_data tap_detection;
    struct inertial_cursor_data inertial_cursor;
};

struct gesture_config {
    const bool circular_scroll, right_side_vertical_scroll, top_side_horizontal_scroll;
    const uint8_t circular_scroll_rim_percent,
        right_side_vertical_scroll_percent, top_side_horizontal_scroll_percent;
    
    struct touch_detection_config touch_detection;
    struct tap_detection_config tap_detection;
    struct inertial_cursor_config inertial_cursor;
};