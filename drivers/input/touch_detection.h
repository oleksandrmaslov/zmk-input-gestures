/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "input_processor_gestures.h"

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

handle_init_t touch_detection_init;
int touch_detection_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state);