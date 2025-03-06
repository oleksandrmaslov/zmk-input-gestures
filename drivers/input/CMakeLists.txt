# Copyright (c) 2024 The ZMK Contributors
# SPDX-License-Identifier: MIT

if (CONFIG_DT_HAS_ZMK_INPUT_PROCESSOR_GESTURES_ENABLED AND NOT ZMK_POINTING)
    message( FATAL_ERROR "You don't have ZMK_POINTING configured, but zmk_input_gestures depends on it." )    
endif()

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURES input_processor_gestures.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURES tap_detection.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURES touch_detection.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURES circular_scroll.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURES inertial_cursor.c)
