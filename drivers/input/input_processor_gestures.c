/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_gestures

#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);


static int handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    return ZMK_INPUT_PROC_CONTINUE;
}


static int gestures_init(const struct device *dev) {
    return 0;
}

static const struct zmk_input_processor_driver_api gestures_driver_api = {
    .handle_event = handle_event,
};

#define GESTURES_INST(n)                                                                                 \
    DEVICE_DT_INST_DEFINE(n, gestures_init, NULL, NULL,                               \
                          NULL, POST_KERNEL,                                   \
                          CONFIG_INPUT_PINNACLE_INIT_PRIORITY, &gestures_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GESTURES_INST)