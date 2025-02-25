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

#define PI 3.14159265358979323846

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

static bool is_touch_on_perimeter(uint16_t x, uint16_t y, struct gesture_config *config, struct gesture_data *data) {
    uint32_t squared_distance = (x - data->circular_scroll.half_width) * (x - data->circular_scroll.half_width) + 
                            (y - data->circular_scroll.half_width) * (y - data->circular_scroll.half_width);
    return (squared_distance >= data->circular_scroll.inner_radius_squared && 
            squared_distance <= data->circular_scroll.outer_radius_squared);
}

static uint16_t calculate_angle(uint16_t x, uint16_t y, struct gesture_config *config, struct gesture_data *data) {
    double angleRadians = atan2(x - data->circular_scroll.half_width, y - data->circular_scroll.half_height);
    double angleDegrees = angleRadians * (180.0 / PI);
    if (angleDegrees < 0) {
        angleDegrees += 360.0;
    }
    return angleDegrees;
}

static double normalizeAngleDifference(uint16_t angle1, uint16_t angle2) {
    double difference = angle2 - angle1;
    while (difference > 180.0) {
        difference -= 360.0;
    }
    while (difference < -180.0) {
        difference += 360.0;
    }

    return difference;
}

int circular_scroll_handle_start(const struct device *dev, uint16_t x, uint16_t y, struct input_event *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;
    if (!config->circular_scroll.enabled) {
        return -1;
    }

    if (event->type == INPUT_EV_ABS && is_touch_on_perimeter(x, y, config, data)) {
        data->circular_scroll.is_tracking = true;
        data->circular_scroll.previous_angle = calculate_angle(x, y, config, data);
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
        uint16_t current_angle = calculate_angle(x, y, config, data);

        event->code = INPUT_REL_WHEEL;
        event->type = INPUT_EV_REL;
        event->value = normalizeAngleDifference(current_angle, data->circular_scroll.previous_angle);

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
        config->circular_scroll.height);

    if (!config->circular_scroll.enabled) {
        return -1;
    }

    // precalculate stuff so the touch events are less expensive
    data->circular_scroll.half_width = config->circular_scroll.width / 2;
    data->circular_scroll.half_height = config->circular_scroll.height / 2;

    uint16_t threshold = (config->circular_scroll.width + config->circular_scroll.height) * (config->circular_scroll.circular_scroll_rim_percent / 2);
    threshold /= 100;
    uint16_t inner_radius = data->circular_scroll.half_width - threshold;
    data->circular_scroll.inner_radius_squared = inner_radius * inner_radius;
    data->circular_scroll.outer_radius_squared = data->circular_scroll.half_width * data->circular_scroll.half_width;

    return 0;
}
