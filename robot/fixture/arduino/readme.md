# TOF Sensor Calibration

Currently, we are using [Arduino Mega 2560 Rev3](https://docs.arduino.cc/hardware/mega-2560) development board to calibrate the ToF sensor.
The development board can be programmed using the [Arduino IDE](https://www.arduino.cc/en/software).
The output of the calibration is shown using a [0.96" OLED SSD1306](https://www.adafruit.com/product/326) display.

## Install Dependencies

- Install the [Arduino IDE](https://www.arduino.cc/en/software) on your system.
- There are 3 libraries used in the calibrator: `Adafruit_GFX_Library`, `Adafruit_GFX_Library`, and `Adafruit_VL53L0X`.
  Compress these libraries in zip format.
- Open Arduino IDE and add these libraries by navigating to `Sketch > Include Library > Add . ZIP Library`.

## Build and Run

- Open the `tof_calibration.ino` file in Arduino IDE by navigating to `File > Open`.
- Connect the Mega2560 Pro board to your system.
- Select the board, processor, port and programmer by navigating to `Tools`.

```
Tools > Board > Arduino AVR Boards > Arduino Mega or Mega 2560
Tools > Processor > ATMega2560 (Mega 2560)
Tools > Port > Choose a USB Port
Tools > Programmer > AVR ISP
```
