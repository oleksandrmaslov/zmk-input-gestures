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

/**
 * Проверяет, находится ли точка касания в пределах кольцевой области прокрутки.
 */
static bool is_touch_on_perimeter(struct gesture_event_t *event,
                                  struct gesture_config *config,
                                  struct gesture_data *data) {
    int32_t dx = event->x - data->circular_scroll.half_width;
    int32_t dy = event->y - data->circular_scroll.half_height;
    uint32_t squared_distance = dx * dx + dy * dy;
    return (squared_distance >= data->circular_scroll.inner_radius_squared &&
            squared_distance <= data->circular_scroll.outer_radius_squared);
}

/**
 * Вычисляет угол в градусах от центра области до точки касания с использованием float.
 */
static uint16_t calculate_angle(struct gesture_event_t *event,
                                struct gesture_config *config,
                                struct gesture_data *data) {
    float dx = (float)(event->x - data->circular_scroll.half_width);
    float dy = (float)(event->y - data->circular_scroll.half_height);
    float angleRadians = atan2f(dx, dy);
    float angleDegrees = angleRadians * (180.0f / PI_F);
    if (angleDegrees < 0) {
        angleDegrees += 360.0f;
    }
    return (uint16_t)angleDegrees;
}

/**
 * Нормализует разницу между двумя углами в диапазоне [-180, 180] с использованием float.
 */
static float normalize_angle_difference(uint16_t prev_angle, uint16_t current_angle) {
    float difference = (float)current_angle - (float)prev_angle;
    while (difference > 180.0f) {
        difference -= 360.0f;
    }
    while (difference < -180.0f) {
        difference += 360.0f;
    }
    return difference;
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
        LOG_DBG("Starting circular scrolling with initial angle %d", data->circular_scroll.previous_angle);
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
        float angle_diff = normalize_angle_difference(data->circular_scroll.previous_angle, current_angle);
        
        /* Применяем коэффициент чувствительности прокрутки.
         * Если в конфигурации scroll_sensitivity задан, то оно умножается на разницу углов.
         * Например, 1.0f означает нормальную чувствительность, 0.5f — меньшую, 1.5f — большую. */
        angle_diff *= config->circular_scroll.scroll_sensitivity;
        
        /* Формируем событие прокрутки */
        event->raw_event_2->code = INPUT_REL_WHEEL;
        event->raw_event_2->type = INPUT_EV_REL;
        event->raw_event_2->value = (int16_t)angle_diff;

        LOG_DBG("Touch event: current_angle=%d, angle_diff=%f (after sensitivity), scroll_value=%d",
                current_angle, angle_diff, event->raw_event_2->value);

        /* Обновляем предыдущий угол */
        data->circular_scroll.previous_angle = current_angle;

        /* Сбрасываем absolute-событие, чтобы предотвратить перемещение указателя */
        event->absolute = false;
        event->raw_event_1->code = 0;
        event->raw_event_1->type = 0;
        event->raw_event_1->value = 0;
    }

    return 0;
}

int circular_scroll_handle_end(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;

    if (data->circular_scroll.is_tracking) {
        data->circular_scroll.is_tracking = false;
        LOG_DBG("Ending circular scrolling");
    }

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

    /* Предварительный расчёт параметров для экономии вычислений */
    data->circular_scroll.half_width = config->circular_scroll.width / 2;
    data->circular_scroll.half_height = config->circular_scroll.height / 2;

    uint16_t average_dim = (config->circular_scroll.width + config->circular_scroll.height) / 2;
    uint16_t threshold = (average_dim * config->circular_scroll.circular_scroll_rim_percent) / 100;
    uint16_t inner_radius = data->circular_scroll.half_width - threshold;
    data->circular_scroll.inner_radius_squared = inner_radius * inner_radius;
    data->circular_scroll.outer_radius_squared = data->circular_scroll.half_width * data->circular_scroll.half_width;

    LOG_DBG("Calculated parameters: half_width=%d, half_height=%d, inner_radius_squared=%d, outer_radius_squared=%d",
            data->circular_scroll.half_width, data->circular_scroll.half_height,
            data->circular_scroll.inner_radius_squared, data->circular_scroll.outer_radius_squared);

    return 0;
}
