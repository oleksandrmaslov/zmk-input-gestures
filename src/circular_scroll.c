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

#define PI_F 3.14159265f

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

/* Проверка попадания касания в активную область кольцевой прокрутки */
static inline bool is_touch_on_perimeter(struct gesture_event_t *event,
                                           struct gesture_config *config,
                                           struct gesture_data *data) {
    int32_t dx = event->x - data->circular_scroll.half_width;
    int32_t dy = event->y - data->circular_scroll.half_height;
    uint32_t squared_distance = dx * dx + dy * dy;
    return (squared_distance >= data->circular_scroll.inner_radius_squared &&
            squared_distance <= data->circular_scroll.outer_radius_squared);
}

/* Вычисление угла (в градусах) относительно центра */
static inline uint16_t calculate_angle(struct gesture_event_t *event,
                                       struct gesture_config *config,
                                       struct gesture_data *data) {
    float dx = (float)(event->x - data->circular_scroll.half_width);
    float dy = (float)(event->y - data->circular_scroll.half_height);
    float angle = atan2f(dx, dy) * (180.0f / PI_F);
    if (angle < 0) {
        angle += 360.0f;
    }
    return (uint16_t)angle;
}

/* Нормализация разницы углов в диапазоне [-180, 180] */
static inline float normalize_angle_difference(uint16_t prev_angle, uint16_t current_angle) {
    float diff = (float)current_angle - (float)prev_angle;
    while (diff > 180.0f) {
        diff -= 360.0f;
    }
    while (diff < -180.0f) {
        diff += 360.0f;
    }
    return diff;
}

int circular_scroll_handle_start(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;
    if (!config->circular_scroll.enabled || !event->absolute) {
        return -1;
    }

    if (is_touch_on_perimeter(event, config, data)) {
        data->circular_scroll.is_tracking = true;
        data->circular_scroll.previous_angle = calculate_angle(event, config, data);
        LOG_DBG("Starting circular scrolling with angle %d", data->circular_scroll.previous_angle);
    }

    return 0;
}

int circular_scroll_handle_touch(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;

    if (!config->circular_scroll.enabled || !data->circular_scroll.is_tracking) {
        return -1;
    }

    if (event->absolute) {
        uint16_t current_angle = calculate_angle(event, config, data);
        /* Обнуление первого сырого события */
        event->raw_event_1->code = 0;
        event->raw_event_1->type = 0;
        event->raw_event_1->value = 0;

        float angle_diff = normalize_angle_difference(data->circular_scroll.previous_angle, current_angle);
        /* Применяем коэффициент чувствительности */
        angle_diff *= config->circular_scroll.scroll_sensitivity;

        event->raw_event_2->code = INPUT_REL_WHEEL;
        event->raw_event_2->type = INPUT_EV_REL;
        event->raw_event_2->value = (int16_t)angle_diff;

        data->circular_scroll.previous_angle = current_angle;
    }

    return 0;
}

int circular_scroll_handle_end(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    data->circular_scroll.is_tracking = false;
    return 0;
}

int circular_scroll_init(const struct device *dev) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;
    LOG_DBG("circular_scroll: %s, rim_percent: %d, width: %d, height: %d, sensitivity: %f",
            config->circular_scroll.enabled ? "yes" : "no",
            config->circular_scroll.circular_scroll_rim_percent,
            config->circular_scroll.width,
            config->circular_scroll.height,
            config->circular_scroll.scroll_sensitivity);

    if (!config->circular_scroll.enabled) {
        return -1;
    }

    /* Предварительный расчёт параметров для экономии вычислительных ресурсов */
    data->circular_scroll.half_width = config->circular_scroll.width / 2;
    data->circular_scroll.half_height = config->circular_scroll.height / 2;

    uint16_t threshold = (((config->circular_scroll.width + config->circular_scroll.height) / 2) *
                          config->circular_scroll.circular_scroll_rim_percent) / 100;
    uint16_t inner_radius = data->circular_scroll.half_width - threshold;
    data->circular_scroll.inner_radius_squared = inner_radius * inner_radius;
    data->circular_scroll.outer_radius_squared = data->circular_scroll.half_width * data->circular_scroll.half_width;

    return 0;
}
