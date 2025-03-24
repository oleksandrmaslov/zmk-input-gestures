#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "input_processor_gestures.h"
#include "circular_scroll.h"

#define PI 3.14159265358979323846f

LOG_MODULE_DECLARE(gestures, CONFIG_ZMK_LOG_LEVEL);

static bool is_touch_on_perimeter(struct gesture_event_t *event, struct gesture_config *config, struct gesture_data *data) {
    uint32_t squared_distance = (event->x - data->circular_scroll.half_width) * (event->x - data->circular_scroll.half_width) + 
                                (event->y - data->circular_scroll.half_height) * (event->y - data->circular_scroll.half_height);
    return (squared_distance >= data->circular_scroll.inner_radius_squared && 
            squared_distance <= data->circular_scroll.outer_radius_squared);
}

static uint16_t calculate_angle(struct gesture_event_t *event, struct gesture_config *config, struct gesture_data *data) {
    float angleRadians = atan2f(event->x - data->circular_scroll.half_width, event->y - data->circular_scroll.half_height);
    float angleDegrees = angleRadians * (180.0f / PI);
    if (angleDegrees < 0) {
        angleDegrees += 360.0f;
    }
    return (uint16_t)angleDegrees;
}

static float normalizeAngleDifference(uint16_t angle1, uint16_t angle2) {
    float difference = (float)angle2 - (float)angle1;
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
        LOG_DBG("starting circular scrolling with angle %d!", data->circular_scroll.previous_angle);
    }

    return 0;
}

int circular_scroll_handle_touch(const struct device *dev, struct gesture_event_t *event) {
    struct gesture_config *config = (struct gesture_config *)dev->config;
    struct gesture_data *data = (struct gesture_data *)dev->data;

    if (!config->circular_scroll.enabled || 
        !data->circular_scroll.is_tracking) {
        return -1;
    }

    if (event->absolute) {
        uint16_t current_angle = calculate_angle(event, config, data);
        event->raw_event_1->code = 0;
        event->raw_event_1->type = 0;
        event->raw_event_1->value = 0;

        event->raw_event_2->code = INPUT_REL_WHEEL;
        event->raw_event_2->type = INPUT_EV_REL;
        event->raw_event_2->value = normalizeAngleDifference(current_angle, data->circular_scroll.previous_angle);
        
        event->x = data->circular_scroll.half_width;
        event->y = data->circular_scroll.half_height;
        
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
    LOG_DBG("circular_scroll: %s, rim_percent: %d, width: %d, height: %d", 
        config->circular_scroll.enabled ? "yes" : "no", 
        config->circular_scroll.circular_scroll_rim_percent,
        config->circular_scroll.width,
        config->circular_scroll.height);

    if (!config->circular_scroll.enabled) {
        return -1;
    }

    // Предварительный расчёт для оптимизации обработки событий касания
    data->circular_scroll.half_width = config->circular_scroll.width / 2;
    data->circular_scroll.half_height = config->circular_scroll.height / 2;

    uint16_t threshold = (config->circular_scroll.width + config->circular_scroll.height) * (config->circular_scroll.circular_scroll_rim_percent / 2);
    threshold /= 100;
    uint16_t inner_radius = data->circular_scroll.half_width - threshold;
    data->circular_scroll.inner_radius_squared = inner_radius * inner_radius;
    data->circular_scroll.outer_radius_squared = data->circular_scroll.half_width * data->circular_scroll.half_width;

    return 0;
}
