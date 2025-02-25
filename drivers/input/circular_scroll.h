/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "input_processor_gestures.h"

struct circular_scroll_data {
    bool is_tracking;
    uint32_t previous_angle;
    gesture_data *all;

    uint16_t threshold, half_width, half_height;
};

struct circular_scroll_config {
    const bool enabled;
    const uint16_t width, height;
    const uint8_t circular_scroll_rim_percent;
};

handle_init_t circular_scroll_init;
handle_touch_t circular_scroll_handle_start;
handle_touch_t  circular_scroll_handle_touch;
handle_touch_end_t circular_scroll_handle_end;

