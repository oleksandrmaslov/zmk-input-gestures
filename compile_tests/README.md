# compile tests 

These shields provide several different kinds of ways to include the cirque on a keyboard.
I'm using them to check that compilation still works as expected.

# Shields

- **UsbOnly01**:
    - wired
    - i2c cirque
    - on central (right)
    - usb only - ble disabled
    --> Left (peripheral) **should NOT** include the input stuff
    --> Right (central) **should** include the input stuff

- **UsbOnly02**:
    - wired
    - i2c cirque
    - on peripheral (right)
    - usb only - ble disabled
    --> Left (central) **should NOT** include the input stuff
    --> Right (peripheral) **should** include the input stuff

- **Ble01**:
    - wireless
    - spi cirque
    - on central (right)
    - right is central
    - ble only - usb disabled
    --> Left (peripheral) **should NOT** include the input stuff
    --> Right (central) **should** include the input stuff
- **Ble02**:
    - wireless
    - spi cirque
    - on peripheral (right)
    - left is central
    - ble only - usb disabled
    --> Left (central) **should NOT** include the input stuff
    --> Right (peripheral) **should** include the input stuff
    