/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX_License_Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_gestures

#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "input_processor_gestures.h"

#include "touch_detection.h"
#include "tap_detection.h"
#include "circular_scroll.h"
#include "inertial_cursor.h"


LOG_MODULE_REGISTER(gestures, CONFIG_ZMK_LOG_LEVEL);

static void handle_init(const struct device *dev) {
    touch_detection_init(dev);
    tap_detection_init(dev);
    circular_scroll_init(dev);
    inertial_cursor_init(dev);
}

static int handle_touch_start(const struct device *dev, uint16_t x, uint16_t y, struct input_event *event) {
    LOG_DBG("handle_touch_start");
    circular_scroll_handle_start(dev, x, y, event);
    tap_detection_handle_start(dev, x, y, event);
    inertial_cursor_handle_touch_start(dev, x, y, event);
    return 0;
}

static int handle_touch(const struct device *dev, uint16_t x, uint16_t y, struct input_event *event) {
    LOG_DBG("handle_touch_ongoing");
    circular_scroll_handle_touch(dev, x, y, event);
    tap_detection_handle_touch(dev, x, y, event);
    inertial_cursor_handle_touch(dev, x, y, event);
    return 0;
}

static int handle_touch_end(const struct device *dev) {
    LOG_DBG("handle_touch_end");
    circular_scroll_handle_end(dev);
    inertial_cursor_handle_end(dev);
    return 0;
}

static int gestures_init(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;

    data->dev = dev;
    data->touch_detection.all = data;
    data->tap_detection.all = data;
    data->circular_scroll.all = data;
    data->inertial_cursor.all = data;

    handle_init(dev);
    return 0;
}

// use touch_detection.c#touch_detection_handle_event as api
static const struct zmk_input_processor_driver_api gestures_driver_api = {
    .handle_event = touch_detection_handle_event,
};

#define GESTURES_INST(n)                                                                                    \
    static struct gesture_data gesture_data_##n = {                                                         \
    };                                                                                                      \
    static const struct tap_detection_config tap_detection_config_##n = {                                   \
        .enabled = DT_INST_PROP(n, tap_detection),                                                          \
        .tap_timout_ms = DT_INST_PROP(n, tap_timout_ms),                                                    \
        .prevent_movement_during_tap = DT_INST_PROP(n, prevent_movement_during_tap),                        \
    };                                                                                                      \
    static const struct touch_detection_config touch_detection_config_##n = {                               \
        .wait_for_new_position_ms = DT_INST_PROP(n, wait_for_new_position_ms),                              \
    };                                                                                                      \
    static const struct circular_scroll_config circular_scroll_config_##n = {                               \
        .enabled = DT_INST_PROP(n, circular_scroll),                                                        \
        .circular_scroll_rim_percent = DT_INST_PROP(n, circular_scroll_rim_percent),                        \
        .width = DT_INST_PROP(n, circular_scroll_width),                                                    \
        .height = DT_INST_PROP(n, circular_scroll_height),                                                  \
    };                                                                                                      \
    static const struct inertial_cursor_config inertial_cursor_config_##n = {                               \
        .enabled = DT_INST_PROP(n, inertial_cursor),                                                        \
        .velocity_threshold = DT_INST_PROP(n, inertial_cursor_velocity_threshold),                          \
        .decay_percent = DT_INST_PROP(n, inertial_cursor_decay_percent),                                    \
    };                                                                                                      \
    static const struct gesture_config gesture_config_##n = {                                               \
        .handle_touch_start = &handle_touch_start,                                                          \
        .handle_touch_continue = &handle_touch,                                                             \
        .handle_touch_end = &handle_touch_end,                                                              \
        .tap_detection = tap_detection_config_##n,                                                          \
        .touch_detection = touch_detection_config_##n,                                                      \
        .circular_scroll = circular_scroll_config_##n,                                                      \
        .inertial_cursor = inertial_cursor_config_##n,                                                      \
    };                                                                                                      \
    DEVICE_DT_INST_DEFINE(n, gestures_init, PM_DEVICE_DT_INST_GET(n), &gesture_data_##n,                    \
                          &gesture_config_##n, POST_KERNEL, CONFIG_INPUT_GESTURES_INIT_PRIORITY,            \
                          &gestures_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GESTURES_INST)
