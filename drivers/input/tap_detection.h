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

int tap_detection_init(const struct device *dev);
int tap_detection_handle_start(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state);
int tap_detection_handle_touch(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state);
