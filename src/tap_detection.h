/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "input_processor_gestures.h"

struct tap_detection_data {
    bool is_waiting_for_tap;
    struct k_work_delayable tap_timeout_work;
    gesture_data *all;
};

struct tap_detection_config {
    const bool enabled;
    const bool prevent_movement_during_tap;
    const uint8_t tap_timout_ms;
};

handle_init_t tap_detection_init;
handle_touch_t tap_detection_handle_start;
handle_touch_t tap_detection_handle_touch;

