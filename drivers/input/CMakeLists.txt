# Copyright (c) 2024 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (CONFIG_DT_HAS_ZMK_INPUT_PROCESSOR_GESTURES_ENABLED AND NOT CONFIG_ZMK_POINTING)
    message( FATAL_ERROR "You don't have ZMK_POINTING configured, but zmk_input_gestures depends on it." )    
endif()

if (CONFIG_ZMK_INPUT_PROCESSOR_GESTURES)
    zephyr_library()

    zephyr_library_sources(input_processor_gestures.c)
    zephyr_library_sources(tap_detection.c)
    zephyr_library_sources(touch_detection.c)
    zephyr_library_sources(circular_scroll.c)
    zephyr_library_sources(inertial_cursor.c)
endif()