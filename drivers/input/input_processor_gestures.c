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

LOG_MODULE_REGISTER(gestures, CONFIG_ZMK_LOG_LEVEL);

/* These are the gestures API - adjust only these when adding new gestures */

/**
    Is called at start time - use it to set up gestures
 */
static void handle_init(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    if (config->tap_detection.enabled) {
        tap_detection_init(dev);
    }
}

/**
    Is called when at the beginning of a touch
 */
static int handle_touch_start(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    tap_detection_handle_start(dev, event, param1, param2, state);

    if (event->type == INPUT_EV_ABS) {
        if (event->code == INPUT_ABS_X) {
            data->touch_detection.previous_abs_x = event->value;
        } else if (event->code == INPUT_ABS_Y) {
            data->touch_detection.previous_abs_y = event->value;
        }
    } else if (event->type == INPUT_EV_REL) {
        if (event->code == INPUT_REL_X) {
            data->touch_detection.previous_rel_x = event->value;
        } else if (event->code == INPUT_REL_Y) {
            data->touch_detection.previous_rel_y = event->value;
        }
    }

    return 0;
}

/**
    Is called when the touch has ended
 */
static int handle_touch_end(const struct device *dev) {
    return 0;
}

/**
    Is called for ongoing touch events
 */
static int handle_touch(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    tap_detection_handle_touch(dev, event, param1, param2, state);

    uint16_t value = event->value;

    if (event->type == INPUT_EV_ABS) {
        if (event->code == INPUT_ABS_X) {
            data->touch_detection.previous_abs_x = value;
        } else if (event->code == INPUT_ABS_Y) {
            data->touch_detection.previous_abs_y = value;
        }
    } else if (event->type == INPUT_EV_REL) {
        if (event->code == INPUT_REL_X) {
            data->touch_detection.previous_rel_x = value;
        } else if (event->code == INPUT_REL_Y) {
            data->touch_detection.previous_rel_y = value;
        }
    }

    return 0;
}

/* These are an internal API used to detect beginnings and ends of a touch */

static int handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;

    uint32_t current_time = k_uptime_get();
    k_work_reschedule(&data->touch_detection.touch_end_timeout_work, K_MSEC(config->touch_detection.wait_for_new_position_ms));
    if (!data->touch_detection.touching){
        data->touch_detection.touching = true;
        handle_touch_start(dev, event, param1, param2, state);
    } else {
        handle_touch(dev, event, param1, param2, state);
    }

    data->touch_detection.last_touch_timestamp = current_time;

    return ZMK_INPUT_PROC_CONTINUE;
}

static void touch_end_timeout_callback(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct touch_detection_data *data = CONTAINER_OF(d_work, struct touch_detection_data, touch_end_timeout_work);
    const struct device *dev = data->all->dev;
    
    data->touching = false;
    handle_touch_end(dev);
}

static int gestures_init(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    LOG_INF("tap_detection: %s", config->tap_detection.enabled ? "yes" : "no");

    data->dev = dev;
    data->touch_detection.last_touch_timestamp = k_uptime_get();
    data->touch_detection.all = data;
    data->tap_detection.all = data;

    k_work_init_delayable(&data->touch_detection.touch_end_timeout_work, touch_end_timeout_callback);

    handle_init(dev);
    return 0;
}

static const struct zmk_input_processor_driver_api gestures_driver_api = {
    .handle_event = handle_event,
};

#define GESTURES_INST(n)                                                                                    \
    static struct gesture_data gesture_data_##n = {                                                         \
    };                                                                                                      \
    static const struct tap_detection_config tap_detection_config_##n = {                                         \
        .enabled = DT_INST_PROP(n, tap_detection),                                                      \
        .tap_timout_ms = DT_INST_PROP(n, tap_timout_ms),                                                      \
        .prevent_movement_during_tap = DT_INST_PROP(n, prevent_movement_during_tap), \
    };                                                                                                      \
    static const struct touch_detection_config touch_detection_config_##n = {                                         \
        .wait_for_new_position_ms = DT_INST_PROP(n, wait_for_new_position_ms),                                                      \
    };                                                                                                      \
    static const struct gesture_config gesture_config_##n = {                                               \
        .tap_detection = tap_detection_config_##n,                                               \
        .touch_detection = touch_detection_config_##n,                                               \
    };                                                                                                          \
    DEVICE_DT_INST_DEFINE(n, gestures_init, PM_DEVICE_DT_INST_GET(n), &gesture_data_##n,                    \
                          &gesture_config_##n, POST_KERNEL, CONFIG_INPUT_GESTURES_INIT_PRIORITY,            \
                          &gestures_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GESTURES_INST)
