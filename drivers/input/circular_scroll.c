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

// Define a threshold for the perimeter detection
#define PERIMETER_THRESHOLD 10

static bool is_touch_on_perimeter(uint16_t x, uint16_t y, struct circular_scroll_config config) {
    uint16_t threshold = (config.width + config.height) * (config.circular_scroll_rim_percent / 2);
    threshold /= 100;
    return (x < threshold || x > (config.width - threshold) || y < threshold || y > (config.height - threshold));
}

// Function to calculate the angle of the touch point relative to the center
static uint32_t calculate_angle(uint16_t x, uint16_t y, int center_x, int center_y) {
    return  (uint32_t) (atan2(y - center_y, x - center_x) * 100.0f);
}

int circular_scroll_handle_start(const struct device *dev, uint16_t x, uint16_t y, struct input_event *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;
    
    if (!config->circular_scroll.enabled) {
        return -1;
    }

    // Check if the touch starts on the perimeter
    if (event->type == INPUT_EV_ABS && is_touch_on_perimeter(x, y, config->circular_scroll)) {
        data->circular_scroll.is_tracking = true;
        data->circular_scroll.previous_angle = calculate_angle(x, y, 
            config->circular_scroll.width / 2, 
            config->circular_scroll.height / 2);
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
        uint32_t current_angle = calculate_angle(x, y, 
            config->circular_scroll.width / 2, 
            config->circular_scroll.height / 2);
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
    LOG_INF("circular_scroll: %s, rim_percent: %d, width: %d, height: %d", 
        config->circular_scroll.enabled ? "yes" : "no", 
        config->circular_scroll.circular_scroll_rim_percent,
        config->circular_scroll.width,
        config->circular_scroll.height
        );

    if (!config->circular_scroll.enabled) {
        return -1;
    }

    return 0;
}
