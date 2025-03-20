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

#define PI 3.14159265358979323846f


LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

/* Глобальный флаг, указывающий, что активен режим прокрутки.
 * Этот флаг можно использовать в драйвере трекпада для подавления перемещения указателя.
 */
volatile bool scroll_mode_active = false;

/**
 * Проверяет, находится ли касание в пределах кольцевой области.
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
 * Вычисляет угол (в градусах) касания относительно центра.
 */
static uint16_t calculate_angle(struct gesture_event_t *event,
                                struct gesture_config *config,
                                struct gesture_data *data) {
    float dx = event->x - data->circular_scroll.half_width;
    float dy = event->y - data->circular_scroll.half_height;
    float angleRadians = atan2f(dx, dy);
    float angleDegrees = angleRadians * (180.0f / PI);
    if (angleDegrees < 0) {
        angleDegrees += 360.0f;
    }
    return (uint16_t)angleDegrees;
}

/**
 * Нормализует разницу между двумя углами в диапазоне [-180, 180].
 */
static float normalize_angle_difference(uint16_t prev_angle, uint16_t curr_angle) {
    float diff = (float)curr_angle - (float)prev_angle;
    while (diff > 180.0f) {
        diff -= 360.0f;
    }
    while (diff < -180.0f) {
        diff += 360.0f;
    }
    return diff;
}

/**
 * Обработка начала жеста прокрутки.
 * Если касание находится в пределах периметра, активируется режим скролла.
 */
int circular_scroll_handle_start(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    struct gesture_config *config = (struct gesture_config *)dev->config;
    if (!config->circular_scroll.enabled || !event->absolute) {
        return -1;
    }
    if (is_touch_on_perimeter(event, config, data)) {
        data->circular_scroll.is_tracking = true;
        data->circular_scroll.previous_angle = calculate_angle(event, config, data);
        scroll_mode_active = true;  /* Включаем режим прокрутки */
        LOG_DBG("Начало кругового скролла с углом %d", data->circular_scroll.previous_angle);
    }
    return 0;
}

/**
 * Обработка событий касания во время активного скролла.
 * Здесь отменяются события перемещения (raw_event_1), а вычисленная разница углов
 * передаётся как событие прокрутки (raw_event_2).
 */
int circular_scroll_handle_touch(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;
    if (!config->circular_scroll.enabled || !data->circular_scroll.is_tracking) {
        return -1;
    }
    if (event->absolute) {
        uint16_t current_angle = calculate_angle(event, config, data);
        /* Отменяем любые события перемещения */
        event->raw_event_1->code = 0;
        event->raw_event_1->type = 0;
        event->raw_event_1->value = 0;

        /* Вычисляем разницу углов и генерируем событие скролла */
        float angle_diff = normalize_angle_difference(data->circular_scroll.previous_angle, current_angle);
        event->raw_event_2->code = INPUT_REL_WHEEL;
        event->raw_event_2->type = INPUT_EV_REL;
        event->raw_event_2->value = (int)angle_diff;

        data->circular_scroll.previous_angle = current_angle;
    }
    return 0;
}

/**
 * Завершение режима прокрутки.
 */
int circular_scroll_handle_end(const struct device *dev) {
    struct gesture_data *data = (struct gesture_data *)dev->data;
    if (data->circular_scroll.is_tracking) {
        data->circular_scroll.is_tracking = false;
    }
    scroll_mode_active = false;  /* Выключаем режим прокрутки */
    return 0;
}

/**
 * Инициализация параметров кругового скролла.
 * Предвычисляются значения для внутреннего и внешнего радиусов.
 */
int circular_scroll_init(const struct device *dev) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;
    LOG_DBG("circular_scroll: %s, rim_percent: %d, width: %d, height: %d", 
            config->circular_scroll.enabled ? "yes" : "no", 
            config->circular_scroll.circular_scroll_rim_percent,
            config->circular_scroll.width,
            config->circular_scroll.height);

    if (!config->circular_scroll.enabled) {
        return -1;
    }

    data->circular_scroll.half_width = config->circular_scroll.width / 2;
    data->circular_scroll.half_height = config->circular_scroll.height / 2;

    uint16_t threshold = ((config->circular_scroll.width + config->circular_scroll.height) *
                          (config->circular_scroll.circular_scroll_rim_percent / 2)) / 100;
    uint16_t inner_radius = data->circular_scroll.half_width - threshold;
    data->circular_scroll.inner_radius_squared = inner_radius * inner_radius;
    data->circular_scroll.outer_radius_squared = data->circular_scroll.half_width * data->circular_scroll.half_width;

    return 0;
}
