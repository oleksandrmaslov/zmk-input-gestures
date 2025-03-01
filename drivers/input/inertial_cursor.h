#pragma once

#include "input_processor_gestures.h"

struct inertial_cursor_data {
    struct k_work_delayable inertial_work;
    uint16_t previous_x, previous_y;
    double velocity_x, velocity_y;
    double velocity_decay;
    gesture_data *all;
};

struct inertial_cursor_config {
    const bool enabled;
    const uint16_t velocity_threshold;
    const uint8_t decay_percent;
};

handle_init_t inertial_cursor_init;
handle_touch_t inertial_cursor_handle_touch_start;
handle_touch_t inertial_cursor_handle_touch;
handle_touch_end_t inertial_cursor_handle_end;
