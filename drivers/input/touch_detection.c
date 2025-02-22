/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include "input_processor_gestures.h"

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

static void remember_previous(struct touch_detection_data data, struct input_event *event) {
    if (event->type == INPUT_EV_ABS) {
        if (event->code == INPUT_ABS_X) {
            data.previous_abs_x = event->value;
        } else if (event->code == INPUT_ABS_Y) {
            data.previous_abs_y = event->value;
        }
    } else if (event->type == INPUT_EV_REL) {
        if (event->code == INPUT_REL_X) {
            data.previous_rel_x = event->value;
        } else if (event->code == INPUT_REL_Y) {
            data.previous_rel_y = event->value;
        }
    }
}

int touch_detection_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;
    k_work_reschedule(&data->touch_detection.touch_end_timeout_work, K_MSEC(config->touch_detection.wait_for_new_position_ms));

    if (!data->touch_detection.touching){
        data->touch_detection.touching = true;
        config->handle_touch_start(dev, event, param1, param2, state);
    } else {
        config->handle_touch_continue(dev, event, param1, param2, state);
    }
    remember_previous(data->touch_detection, event);

    data->touch_detection.last_touch_timestamp = k_uptime_get();

    return ZMK_INPUT_PROC_CONTINUE;
}

void touch_end_timeout_callback(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct touch_detection_data *data = CONTAINER_OF(d_work, struct touch_detection_data, touch_end_timeout_work);
    const struct device *dev = data->all->dev;
    struct gesture_config *config = (struct gesture_config *)dev->config;
    
    data->touching = false;
    config->handle_touch_end(dev);
}

int touch_detection_init(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    k_work_init_delayable(&data->touch_detection.touch_end_timeout_work, touch_end_timeout_callback);
    return 0;
}
