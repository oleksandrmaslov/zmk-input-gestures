# ZMK INPUT GESTURES

This repository contains a collection of gestures that interpret data from p.e. touchpads.

Some of these gestures require absolute positioning data, which isn't natively supported by ZMK,
but a modified driver for the cirque touchpad is available.

Although most of this code seems to work fine, please note that
it's very new and there absolutely can be bugs that might crash
your keyboard, so that you need to physically press the reset button so you can p.e. flash a previously known uf2 file. That won't be a problem for most people, but not all boards have easy access to these buttons.

To be clear: right now, this is not for the faint of heart.

**Before you start, you should make sure that you have a working
input device by following this: https://zmk.dev/docs/features/pointing**

## Table of Contents

* [Relative and Absolute Mode](#relative-and-absolute-mode)
* [Supported gestures](#supported-gestures)
* [Planned gestures](#planned-gestures)
* [Installation &amp; Usage](#installation--usage)
  * [west.yml](#westyml)
  * [Import the dependencies](#import-the-dependencies)
  * [Activate absolute mode for cirque glidepoint](#activate-absolute-mode-for-cirque-glidepoint)
  * [Translate absolute to relative positions](#translate-absolute-to-relative-positions)
  * [Configure some gestures and add them](#configure-some-gestures-and-add-them)
  * [Increase the Stack Size](#increase-the-stack-size)
* [Gestures](#gestures)
  * [Tap Detection (Absolute and Relative Mode)](#tap-detection-absolute-and-relative-mode)
  * [Inertial Cursor (Absolute and Relative Mode - Not implemented.)](#inertial-cursor-absolute-and-relative-mode---not-implemented)
  * [Circular Scroll (Absolute Mode only!)](#circular-scroll-absolute-mode-only)
  * [Right-Side Vertical Scroll (Absolute Mode only! Not implemented.)](#right-side-vertical-scroll-absolute-mode-only-not-implemented)
  * [Top-Side Horizontal Scroll (Absolute Mode only! Not implemented.)](#top-side-horizontal-scroll-absolute-mode-only-not-implemented)
  * [Wait for New Position](#wait-for-new-position)
* [Configuration options for absolute mode in cirque glidepad driver](#configuration-options-for-absolute-mode-in-cirque-glidepad-driver)
  * [Absolute Mode](#absolute-mode)
* [Troubleshooting](#troubleshooting)
  * [My build fails with Assembler messages Error: missing expression](#my-build-fails-with-assembler-messages-error-missing-expression)

## Relative and Absolute Mode

In **relative mode**, the input device reports the change that happened
relative to the previous state. This is the normal behavior for a 
mouse: moving the mouse up will report only that the mouse has
moved an amount of pixels up.

In **absolute mode** however, the input device reports the position
of a touch on a touchpad: the top left might be x=1024/y=998 or so.

In order to detect whether or not a touch has happened on the outer perimeter of a touchpad, it's necessary to know the absolute position of the touch, so some of the gestures are 
only doing anything if you're using absolute mode.

As of now, ZMK doesn't support absolute mode natively, so to use the gestures that depend on it, a bit more work is necessary. But not much: the pieces are all there, you just need to put them in the right places.

## Supported gestures

- Tap (all modes): translate quick touches to a mouse click
- Circular Scroll (absolute mode only): translate angular movement to scroll events

## Planned gestures

- Inertial Cursor (all modes): keep the cursor moving after the touch ends
- Right side vertical scroll (absolute mode only): translate vertical movement to vertical scroll events
- Top side horizontal scroll (absolute mode only): translate horizontal movement to horizontal scroll events

## Installation & Usage

To use these gestures, there are several steps necessary:
- adjust the `west.yml` to make new dependencies available
- import the dependencies into your configuration files
- use the dependencies as input listeners
- possibly increase the stack size

We'll go through these steps one by one now.

### Adjust west.yml

Depending on whether or not you want to use gestures that rely on absolute mode,
you need to add one or more of these:

- `zmk-input-gestures`: this repo that contains the actual gestures
- `cirque-input-module`: an adjusted version of the normal driver that includes support of absolute mode for the cirque glidepoint. This won't hurt in case you switch to relative mode. **If you use something else than the cirque glidepoint, you're on your own to activate absolute mode.**
- `zmk-input-gestures`: contains an input processor to translate absolute positions to relative positions. This is necessary because ZMK won't interpret absolute positions at all. This won't hurt in case you switch to relative mode.

Add the necessary repositories to your `west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    # Add this:
    - name: halfdane
      url-base: https://github.com/halfdane/
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    # Add this:
    - name: zmk-input-gestures
      remote: halfdane
      revision: main
    # Add these if you want to use absolute mode:
    - name: zmk-input-processors
      remote: halfdane
      revision: main
    - name: cirque-input-module
      remote: halfdane
      revision: absolute_mode


```

> [!WARNING]  
> Run `west update` if you're building on your local machine (not github actions).

### Import the dependencies
Since you're already using the `cirque-input-module`, you don't need to import that again.

Add the necessary includes to your `dtsi`-file:

```C
// to use gestures this is always necessary:
#include <behaviors/input_processor_gestures.dtsi>

// to translate absolute positions to relative positions. If you don't
// want to use gestures for absolute mode, and you don't want to activate it,
// you can ignore this line
#include <behaviors/input_processor_absolute_to_relative.dtsi>
```

At this point, your firmware should compile just fine and not behave any differently.
Please build and flash to make sure that's true!

### Activate absolute mode for cirque glidepoint

If you don't want to use absolute mode, you can skip this.

Adjust the configuration of your cirque glidepoint to activate absolute mode if you wish to use gestures that depend on it. If you don't want any of these gestures, you can skip this step.

This should be somewhere in your dtsi-files. 
If you don't have such a configuration yet, please make sure your cirque glidepoint works
as expected *without* gestures an absolute mode as described here:  https://zmk.dev/docs/features/pointing

```devicetree

    glidepoint: ... {
        compatible = "cirque,pinnacle";
        
        ...

        // if you used no-taps before, that's not necessary for absolute mode
        // because cirque doesn't detect taps in absolute mode anyways.
        // You can comment the setting or just remove it completely.
        // no-taps;
        
        // Add this to activate absolute mode.
        // Unless you translate absolute to relative positions as described below,
        // your mouse cursor won't move anymore
        absolute-mode;
    };

```

Your firmware should compile fine, but if you activated absolute mode, the mouse cursor shouldn't move anymore. Please build and flash to make sure that's true!

### Translate absolute to relative positions
If you don't want to use absolute mode, you can skip this.

Since your mouse cursor doesn't move anymore, it's time to change that with the input processor `zip_absolute_to_relative`.
You probably already have a place where you've added input processors as described here: https://zmk.dev/docs/keymaps/input-processors.

Add the processor to the list of used input processors:

```devicetree
        input-processors = <
            // Add this. It should come before the inbuilt input processsors that
            // rely on relative mode:
            &zip_absolute_to_relative

            &zip_xy_transform (INPUT_TRANSFORM_XY_SWAP | INPUT_TRANSFORM_Y_INVERT)
            &zip_temp_layer 3 100
        >;
```

Your firmware should compile fine, and the mouse cursor should move again. Please build and flash to make sure that's true!

### Configure some gestures and add them

Add the configuration for the gesture input processor before the input listener that you adjusted in the step before. It should be `compatible = "zmk,input-listener";`

This configuration should be at the top level of your devicetree, not within any brackets or other definitions:

```devicetree
&zip_gestures {
    // Activate gestures and change their configuration as you see fit.
    // for details, see the section below that describes all gestures and
    // their configuration options.

    // This will create a mouseclick if your touch is short enough.
    // if you're running in relative mode and don't use no-taps, this might
    // result in duplicate taps
    tap-detection;
};
```

Add the gesture input processor to the list of input-processors:

```devicetree
        input-processors = <
            // the gesture input processor should come before 
            // zip_absolute_to_relative
            &zip_gestures

            // You have this line only if you're using absolute mode
            &zip_absolute_to_relative

            &zip_xy_transform (INPUT_TRANSFORM_XY_SWAP | INPUT_TRANSFORM_Y_INVERT)
            &zip_temp_layer 3 100
        >;
```

### Increase the Stack Size

You may encounter instabilities when using calculation intense gestures.
I found that increasing stack sizes helps.

Add this to your `.conf` file:

```
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_INPUT_THREAD_STACK_SIZE=4096
```

## Gestures

Activate and configure the gestures by adding the corresponding lines to the predefined `&zip_gesture` container like so:

```plaintext
&zip_gestures {
    tap-detection;
    tap-timout-ms=<120>;
};
```

The given numbers are default values that seem to work well for me. If you don't add a value, the default is used. 
Default value for booleans is `false`.


### Tap Detection (Absolute and Relative Mode)

**Description:**
A touch that doesn't last longer than `tap-timout-ms` is interpreted as a tap (click).

**Configuration Options:**
- `tap-detection;`: Activates the tap detection feature.
- `tap-timout-ms=<120>;`: Sets the timeout in milliseconds for detecting a tap. A lower value requires a quicker tap motion but reduces accidental taps for short movements.
- `prevent_movement_during_tap;`: While determining if the beginning of a touch is a tap, ignore all other movements. This prevents accidental movement away from the tap-target but makes the beginning of regular touches more sluggish.

### Inertial Cursor (Absolute and Relative Mode - Not implemented.)

THIS ISN'T IMPLEMENTED YET!

> **Description:**
> If the cursor moves faster than `inertial-cursor-velocity-threshold-ms` when the touch ends, the cursor keeps moving in the same direction, gradually slowing down by `inertial-cursor-decay-percent`.
> 
> **Configuration Options:**
> - `inertial-cursor;`: Activates the inertial cursor feature.
> - `inertial-cursor-velocity-threshold-ms=<20>;`: Sets the velocity threshold in milliseconds for activating inertial movement. A lower value makes it easier to activate but increases accidental activation.
> - `inertial-cursor-decay-percent=<98>;`: Sets the decay percentage for the cursor's inertial movement. A lower value makes the cursor move longer after the touch ends, mimicking lower friction.

### Circular Scroll (Absolute Mode only!)

**Description:**
If a touch begins on the outer `circular-scroll-rim-percent` percentage of the touchpad, angular movement of the touch is interpreted as scrolling down (clockwise) or up (counter-clockwise).
Make sure you have absolute mode activated, otherwise this won't do anything

**Configuration Options:**
- `circular-scroll;`: Activates the circular scroll feature.
- `circular-scroll-rim-percent=<10>;`: Sets the percentage width of the outer ring of the touchpad that activates circular scroll. A lower value reduces accidental activation during normal usage but requires better targeting to activate.
- `circular-scroll-width=<1024>;`: Sets the width of the touchpad. If your device driver supports scaling to a target interval you should make sure to use the same values. See the [section about the cirque-driver](#configuration-options-for-absolute-mode-in-cirque-glidepad-driver) below, if you want to change this, but you probably shouldn't.
- `circular-scroll-height=<1024>;`: Sets the height of the touchpad. If your device driver supports scaling to a target interval you should make sure to use the same values. See the [section about the cirque-driver](#configuration-options-for-absolute-mode-in-cirque-glidepad-driver) below, if you want to change this, but you probably shouldn't.

### Right-Side Vertical Scroll (Absolute Mode only! Not implemented.)

THIS ISN'T IMPLEMENTED YET

> **Description:**
> If a touch begins on the right `right-side-vertical-scroll-percent` percentage of the touchpad, vertical movement of the touch is interpreted as vertical scrolling.
> 
> **Configuration Options:**
> - `right-side-vertical-scroll;`: Activates the right-side vertical scroll feature.
> - `right-side-vertical-scroll-percent=<10>;`: Sets the percentage width of the right part of the touchpad that activates vertical scroll. A lower value reduces accidental activation during normal usage but requires better targeting to activate.

### Top-Side Horizontal Scroll (Absolute Mode only! Not implemented.)

THIS ISN'T IMPLEMENTED YET!

> **Description:**
> If a touch begins on the top `top-side-horizontal-scroll-percent` percentage of the touchpad, horizontal movement of the touch is interpreted as horizontal scrolling.
> 
> **Configuration Options:**
> - `top-side-horizontal-scroll;`: Activates the top-side horizontal scroll feature.
> - `top-side-horizontal-scroll-percent=<10>;`: Sets the percentage height of the upper part of the touchpad that activates horizontal scroll. A lower value reduces accidental activation during normal usage but requires better targeting to activate.

### Wait for New Position

**Description:**
The time it takes the touchpad to generate a new position in normal usage. Lower values result in better detection when a touch ends but might trigger accidental taps or other gestures if a new position gets reported after the wait time while the current touch is ongoing.

**Configuration Options:**
- `wait-for-new-position-ms=<30>`: Sets the time in milliseconds to wait for a new position. The default value allows reliable tap detection while being quick enough to go unnoticed.


## Configuration options for absolute mode in cirque glidepad driver

### Absolute Mode

**Description:**
Report the position of a touch on the touchpad as absolute positions instead of just the change relative to the previous touch.

**Configuration Options:**
- `absolute-mode;`: Activates the absolute mode
- `absolute-mode-scale-to-width=<1024>;`: Scale reported X-positions so they are in the interval [0-1024].
- `absolute-mode-scale-to-height=<1024>;`: Scale reported X-positions so they are in the interval [0-1024].


## Troubleshooting

### My build fails with `Assembler messages Error: missing expression`
In github actions, it might look like this:
```
/tmp/cc51TXht.s: Assembler messages:
/tmp/cc51TXht.s:1131: Error: missing expression
```

Try activating `ZMK_POINTING` as described in https://zmk.dev/docs/features/pointing

