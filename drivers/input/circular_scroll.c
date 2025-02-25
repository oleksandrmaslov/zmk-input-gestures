/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "input_processor_gestures.h"
#include "circular_scroll.h"

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

static bool is_touch_on_perimeter(uint16_t x, uint16_t y, struct gesture_data *data) {
    return (
        x < data->circular_scroll.threshold || 
        x > (config.width - data->circular_scroll.threshold) || 
        y < data->circular_scroll.threshold ||
        y > (config.height - data->circular_scroll.threshold));
}

// Function to calculate the angle of the touch point relative to the center
static uint32_t calculate_angle(uint16_t x, uint16_t y, struct gesture_data *data) {
    return  (uint32_t) (atan2(y - data->circular_scroll.half_height, x - data->circular_scroll.half_width) * 10.0f);
}

int circular_scroll_handle_start(const struct device *dev, uint16_t x, uint16_t y, struct input_event *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;
    
    if (!config->circular_scroll.enabled) {
        return -1;
    }

    // Check if the touch starts on the perimeter
    if (event->type == INPUT_EV_ABS && is_touch_on_perimeter(x, y, data)) {
        data->circular_scroll.is_tracking = true;
        data->circular_scroll.previous_angle = calculate_angle(x, y, data);
        LOG_DBG("starting circular scrolling with angle %d!", data->circular_scroll.previous_angle);
    }

    return 0;
}

int circular_scroll_handle_touch(const struct device *dev, uint16_t x, uint16_t y, struct input_event *event) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;

    if (!config->circular_scroll.enabled || !data->circular_scroll.is_tracking) {
        return -1;
    }

    if (event->type == INPUT_EV_ABS) {
        uint32_t current_angle = calculate_angle(x, y, data);
        uint32_t angle_diff = current_angle - data->circular_scroll.previous_angle;
        LOG_DBG("Continuing circular scroll. angle: %d, angle diff: %d", current_angle, angle_diff);

        // Convert angular movement to scroll events
        event->code = INPUT_REL_WHEEL;
        event->type = INPUT_EV_REL;
        event->value = angle_diff;

        data->circular_scroll.previous_angle = current_angle;
    }

    return 0;
}

int circular_scroll_handle_end(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;

    if (data->circular_scroll.is_tracking) {
        data->circular_scroll.is_tracking = false;
    }

    return 0;
}

int circular_scroll_init(const struct device *dev) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;
    LOG_INF("circular_scroll: %s, rim_percent: %d, width: %d, height: %d", 
        config->circular_scroll.enabled ? "yes" : "no", 
        config->circular_scroll.circular_scroll_rim_percent,
        config->circular_scroll.width,
        config->circular_scroll.height
        );

    if (!config->circular_scroll.enabled) {
        return -1;
    }

    data->circular_scroll.threshold = (config->circular_scroll.width + config->circular_scroll.height) * (config.circular_scroll_rim_percent / 2);
    data->circular_scroll.threshold /= 100;
    data->circular_scroll.half_width = config->circular_scroll.width / 2;
    data->circular_scroll.half_height = config->circular_scroll.height / 2;

    return 0;
}
