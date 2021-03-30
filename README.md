# Chroduino

This is an Arduino-based chronograph, with I2C OLED (screen) and BLE (Bluetooth, low energy) support.

![OLED demo](/images/oled.jpg)

**NOTE**:
 * work in progress, doesn't seem to work with fast moving things, like BBs or pellets.
 * at the moment, the distance between IR sensors is fixed at 10cm (can be changed in code!)
 * configuration is mostly done by changing `define` values at the top of the Arduino file

### Wiring

This projects uses the IR Break Beam Sensors from [here](https://www.adafruit.com/product/2168), making it easy to wire up.
* all the red wires go to 3v or 5v
* all the black wires go to ground (`GND`)
* one white wire connects to pin 2, the other connects to pin 3 (or any other pins that support interrupts, just be sure to modify `SENSORPIN_1` and `SENSORPIN_2` in the Arduino code)
* depending on your board, you can power it with a 9v battery by connecting the positive terminal to `VIN`, and the negative terminal to `GND`

**NOTE**:
 * this uses interrupts for the sensors, so make sure you connect them to pins that support it (pins 2 and 3 should work, if you're not sure)
 * for the I2C OLED screens to automagically work, connect them to pins 4 (`SDA`) and 5 (`SCL`)

### OLED view

Support for `SSD1306` devices exists, catering for 128x64 and 128x32 screens out of the box, by making sure `#define FEATURE_SCREEN` is enabled at the top of the Arduino file.
Specifying the resolution of your screen determines what address to find the screen on (`0x3C` or `0x3C` respectively).

**NOTE**:
 * these screens normally need `SDA` and `SCL` plugged into pins 4 and 5 respectively.

### BLE

If you have an e.g., Arduino Nano BLE 33, then you can enable it by making sure `#define FEATURE_BLE` is enabled at the top of the Arduino file.
It will advertise itself as a Bluetooth device named `Chroduino` to which you can pair with, and subscribe to be notified when its single float value changes (the last measured time).

##### BLE web view

A very barebones viewer of the speed data using BLE is provided in the `Web` folder.
**NOTE:**: this *requires*
 * HTTPS or binding explicitly to localhost for debugging
    * I use Python 3 as my test server, with `python -m http.server --bind localhost`
 * UUID's to be in lowercase
 * using Chrome or Edge to view (see [here](https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API) for browser compatibility)
    * using my Python server commandline above, browse to `http://localhost:8000/ble_notify.html`

When you first click `Start`, you will get a dialog prompting you which device you'd like to pair with, select `Chroduino`.

![Pairing](/images/pairing.jpg)

With every measurement taken, a new line will be added to the list, which can be erased by click the `Clear` button.
If you find you're getting weird values, check the endianness of the float value.

![Measuring](/images/measuring.jpg)
