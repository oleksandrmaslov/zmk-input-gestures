/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int tap_detection_init(const struct device *dev);
int tap_detection_handle_start(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state);
