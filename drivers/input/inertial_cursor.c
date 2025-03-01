#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zmk/hid.h>
#include <math.h>
#include "input_processor_gestures.h"
#include "inertial_cursor.h"

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

static void inertial_cursor_work_handler(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct inertial_cursor_data *data = CONTAINER_OF(d_work, struct inertial_cursor_data, inertial_work);
    const struct device *dev = data->all->dev;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    LOG_INF("current velocity *100: %d", (int) data->velocity*100);

    if (data->velocity > 0.1) {
        LOG_INF("data->velocity_x: %d, data->velocity_y: %d", data->velocity_x, data->velocity_y);

        data->velocity_x *= data->velocity_decay;
        data->velocity_y *= data->velocity_decay;

        LOG_INF("data->velocity_x: %d, data->velocity_y: %d", data->velocity_x, data->velocity_y);

        zmk_hid_mouse_movement_update(data->velocity_x, data->velocity_y);

        data->velocity *= data->velocity_decay;

        LOG_INF("new velocity*100: %d", (int) data->velocity*100);

        k_work_reschedule(&data->inertial_work, K_MSEC(10));
    }
}

int inertial_cursor_handle_touch(const struct device *dev, uint16_t x, uint16_t y, struct input_event *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    if (!config->inertial_cursor.enabled) {
        return -1;
    }

    data->inertial_cursor.velocity_x = x - data->inertial_cursor.previous_x;
    data->inertial_cursor.velocity_y = y - data->inertial_cursor.previous_y;
    data->inertial_cursor.velocity = sqrt(
        data->inertial_cursor.velocity_x * data->inertial_cursor.velocity_x + 
        data->inertial_cursor.velocity_y * data->inertial_cursor.velocity_y);

    data->inertial_cursor.previous_x = x;
    data->inertial_cursor.previous_y = y;

    return 0;
}

int inertial_cursor_handle_end(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    LOG_ERR("inertial_cursor: %s, current velocity: %d, velocity_threshold: %d, too slow: %s", 
        config->inertial_cursor.enabled ? "yes" : "no",
        (int)data->inertial_cursor.velocity, 
        config->inertial_cursor.velocity_threshold, 
        data->inertial_cursor.velocity <= config->inertial_cursor.velocity_threshold?"yes":"no");

    if (!config->inertial_cursor.enabled || data->inertial_cursor.velocity <= config->inertial_cursor.velocity_threshold) {
        return -1;
    }

    k_work_reschedule(&data->inertial_cursor.inertial_work, K_MSEC(10));

    return 0;
}

int inertial_cursor_init(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;

    LOG_INF("inertial_cursor: %s, velocity_threshold: %d, decay_percent: %d", 
        config->inertial_cursor.enabled ? "yes" : "no", 
        config->inertial_cursor.velocity_threshold,
        config->inertial_cursor.decay_percent);


    if (!config->inertial_cursor.enabled) {
        return -1;
    }

    data->inertial_cursor.velocity_decay = (100 - config->inertial_cursor.decay_percent) / 100.0;

    k_work_init_delayable(&data->inertial_cursor.inertial_work, inertial_cursor_work_handler);
    return 0;
}
