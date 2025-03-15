/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include "input_processor_gestures.h"

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

int tap_detection_handle_start(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;

    if (! config->tap_detection.enabled) {
        return -1;
    }

    k_work_reschedule(&data->tap_detection.tap_timeout_work, K_MSEC(config->tap_detection.tap_timout_ms));
    data->tap_detection.is_waiting_for_tap = true;

    if (config->tap_detection.prevent_movement_during_tap) {
        event->raw_event_1->code = 0;
        event->raw_event_1->type = 0;
        event->raw_event_1->value = 0;

        event->raw_event_2->code = 0;
        event->raw_event_2->type = 0;
        event->raw_event_2->value = 0;
    }

    return 0;
}

int tap_detection_handle_touch(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;

    if (! config->tap_detection.enabled) {
        return -1;
    }

    if (data->tap_detection.is_waiting_for_tap && config->tap_detection.prevent_movement_during_tap) {
        event->raw_event_1->code = 0;
        event->raw_event_1->type = 0;
        event->raw_event_1->value = 0;

        event->raw_event_2->code = 0;
        event->raw_event_2->type = 0;
        event->raw_event_2->value = 0;
    }

    return 0;
}


/* Work Queue Callback */
static void tap_timeout_callback(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct tap_detection_data *data = CONTAINER_OF(d_work, struct tap_detection_data, tap_timeout_work);
    data->is_waiting_for_tap = false;
    if (!data->all->touch_detection.touching) {
        LOG_DBG("tap detected - sending button presses");
        zmk_hid_mouse_button_press(0);
        zmk_endpoints_send_mouse_report();
        zmk_hid_mouse_button_release(0);
        zmk_endpoints_send_mouse_report();
    } else {
        LOG_DBG("time expired but touch is ongoing - it's not a tap");
    }
}

int tap_detection_init(const struct device *dev) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;

    LOG_DBG("tap_detection: %s, timeout in ms: %d, prevent_movement_during_tap: %s", 
        config->tap_detection.enabled ? "yes" : "no", 
        config->tap_detection.tap_timout_ms,
        config->tap_detection.prevent_movement_during_tap ? "yes" : "no");

    if (!config->tap_detection.enabled) {
        return -1;
    }
    k_work_init_delayable(&data->tap_detection.tap_timeout_work, tap_timeout_callback);

    return 0;
}
