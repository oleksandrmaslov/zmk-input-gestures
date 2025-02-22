# Copyright (c) 2024 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURES input_processor_gestures.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_PROCESSOR_GESTURES tap_detection.c)
