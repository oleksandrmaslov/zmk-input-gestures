#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zmk/hid.h>
#include <zmk/usb_hid.h>
#include <math.h>
#include <stdlib.h>
#include "input_processor_gestures.h"
#include "inertial_cursor.h"

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

#define ANIMATE_MSEC 50

static void inertial_cursor_work_handler(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct inertial_cursor_data *data = CONTAINER_OF(d_work, struct inertial_cursor_data, inertial_work);

    LOG_DBG("data->velocity_x: %d, data->velocity_y: %d", 
        (int) data->velocity_x, 
        (int) data->velocity_y);

    data->velocity_x *= data->velocity_decay;
    data->velocity_y *= data->velocity_decay;

    if (abs((int) data->velocity_x) > 0 || abs((int) data->velocity_y) > 0) {
        zmk_hid_mouse_movement_update((int) data->velocity_y, (int) -data->velocity_x);
        zmk_usb_hid_send_mouse_report();
        k_work_reschedule(&data->inertial_work, K_MSEC(ANIMATE_MSEC));
    }
}

int inertial_cursor_handle_touch(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    if (!config->inertial_cursor.enabled) {
        return -1;
    }

    if (event->velocity_x != 0) {
        data->inertial_cursor.velocity_x = event->velocity_x * data->inertial_cursor.velocity_decay;
    }

    if (event->velocity_x != 0) {
        data->inertial_cursor.velocity_y = event->velocity_y * data->inertial_cursor.velocity_decay;
    }

    return 0;
}


int inertial_cursor_handle_touch_start(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    if (!config->inertial_cursor.enabled) {
        return -1;
    }

    k_work_cancel_delayable(&data->inertial_cursor.inertial_work);

    data->inertial_cursor.velocity_x = 0;
    data->inertial_cursor.velocity_y = 0;

    inertial_cursor_handle_touch(dev, event);

    return 0;
}

int inertial_cursor_handle_end(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    if (!config->inertial_cursor.enabled) {
        return -1;
    }

    double magnitude = sqrt(
        data->inertial_cursor.velocity_x * data->inertial_cursor.velocity_x + 
        data->inertial_cursor.velocity_y * data->inertial_cursor.velocity_y);

    LOG_DBG("magnitude: %d, velocity_threshold: %d, too slow: %s", 
        (int)magnitude, 
        (int) config->inertial_cursor.velocity_threshold, 
        magnitude <= config->inertial_cursor.velocity_threshold?"yes":"no");

    if (magnitude <= config->inertial_cursor.velocity_threshold) {
        return -1;
    }
    
    zmk_hid_mouse_movement_set(0, 0);
    zmk_usb_hid_send_mouse_report();

    k_work_reschedule(&data->inertial_cursor.inertial_work, K_MSEC(ANIMATE_MSEC));

    return 0;
}

int inertial_cursor_init(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    LOG_DBG("inertial_cursor: %s, velocity_threshold: %d, decay_percent: %d", 
        config->inertial_cursor.enabled ? "yes" : "no", 
        config->inertial_cursor.velocity_threshold,
        config->inertial_cursor.decay_percent);


    if (!config->inertial_cursor.enabled) {
        return -1;
    }

    data->inertial_cursor.velocity_decay = (100.0 - config->inertial_cursor.decay_percent) / 100.0;
    LOG_ERR("velocity_decay *1000: %d", (int) (data->inertial_cursor.velocity_decay * 1000.0));

    k_work_init_delayable(&data->inertial_cursor.inertial_work, inertial_cursor_work_handler);
    return 0;
}
