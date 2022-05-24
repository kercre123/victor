# TOF Sensor Calibration

Currently, we are using [Arduino Mega 2560 Rev3](https://docs.arduino.cc/hardware/mega-2560) development
board to calibrate the ToF sensor.
The development board can be programmed using the [Arduino IDE](https://www.arduino.cc/en/software).
The output of the calibration is shown using a [0.96" OLED SSD1306](https://www.adafruit.com/product/326)
display.

## Install Dependencies

- Install the [Arduino IDE](https://www.arduino.cc/en/software) on your system.
- There are 4 libraries used in the calibrator: `Adafruit_GFX_Library`, `Adafruit_SSD1306`,
  `Adafruit_VL53L0X`, and `ezButton`.
- Compressed versions of the libraries are included in the repository in the `libraries` folder.
- Delete any existing copies of these libraries from your system by navigating to the
  `Sketchbook Location` of the Arduino IDE.
- You can find your `Sketchbook Location` opening the `Arduino IDE` under
  `Preferences > Sketchbook Location`.
- After deleting the existing copies of the libraries, add the copies of these libraries included in this
  repository by navigating to `Sketch > Include Library > Add .ZIP Library`.

## Build and Run

- Open the `tof_calibration.ino` file in Arduino IDE by navigating to `File > Open`.
- Connect the `Mega 2560 Rev3` development board to your system.
- Select the board, processor, port and programmer by navigating to `Tools`.

```
Tools > Board > Arduino AVR Boards > Arduino Mega or Mega 2560
Tools > Processor > ATMega2560 (Mega 2560)
Tools > Port > Choose a USB Port
Tools > Programmer > Atmel STK500 development board
```

- Burn this onto the development board by navigating to `Sketch > Upload`.
