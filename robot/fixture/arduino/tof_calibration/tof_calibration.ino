#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_VL53L0X.h"
#include "ezButton.h"

// OLED display width and height, in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// OLED pins
#define OLED_DC     8
#define OLED_CS     10
#define OLED_RESET  9
// TOF PINS
#define TOF_POWER 22
// Button pins
#define BUTTON 7
// Pin to disable calibration mode
#define CALIBRATING 6
// Range of the TOF sensor is 2000mm
#define MAX_RANGE 2000
// Button long press time (in milliseconds)
#define LONG_PRESS_TIME 3000
// Error in readings form the sensor
#define ERROR_HARDWARE_FAIL 8187
#define ERROR_PHASE_FAIL 8188
#define ERROR_MIN_RANGE_FAIL 8189
#define ERROR_SIGNAL_FAIL 8190
#define ERROR_SIGMA_FAIL 8191
// Uncomment line below to view debug statements
#define DEBUG

//******* Function definitions *******//
// To show a message on the OLED display
void displayMessage(String msg, uint8_t startX, uint8_t textSize, bool invert = false, bool addDelay = false);
// Initialize TOF sensor
bool initTof(void);
// Check if our TOF sensor is connected
bool scanForTof(void);
// Wait for user to connect the sensor
void waitToAddTof(void);
// Wait for user to remove the sensor
void waitToRemoveTof(void);
// Get reading from TOF sensor
int getReadingFromTof(void);
// Check if average reading from the sensor is within our threshold
bool isAverageReadingOkay();
// Check if button was pressed for a long time
// Method is used to enable reading mode
bool isButtonLongPressed(void);
// Check if button has been pressed down
bool isButtonPressed(void);
// Check if button has been released
bool isButtonReleased(void);
// Print errors we get while getting reading from the sensor
void printError(uint16_t error);

// Global variables
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
ezButton button(BUTTON);
bool buttonPressed = false;

// Boot up init things
void setup() {
  Wire.begin();
  Serial.begin(115200);
  pinMode(TOF_POWER, OUTPUT);
  digitalWrite(TOF_POWER, HIGH);
  pinMode(CALIBRATING, INPUT);

  // Set debounce time to 50 milliseconds
  button.setDebounceTime(50);

  // Initialize the display
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("Failed to init display"));
    while(1); // Don't proceed, loop forever
  }
  display.clearDisplay();
  Serial.println("Calibrator started");
  displayMessage("TOF-CALIB", 10, 2, false, true);
}

/**************************************************************************/
/*!
  Logic that repeats after each run
  The calibration of the TOF sensor consists of 2 parts:
    - Performing calibration using the sensor API
    - Performing a set of ranging to establish that the sensor gives out
      readings within a set threshold after calibration
  A TOF sensor is calibrated only if it passes both the parts.
  Each iteration of the loop is set up to calibrate 1 sensor and implements
  both parts of the calibration described above
*/
/**************************************************************************/
void loop() {
  if (buttonPressed) {
    while (!isButtonReleased()) {
      delay(1);
    }
    buttonPressed = false;
  }
  // Wait for sensor to be plugged-in
  displayMessage("READY", 30, 2);
  // Enable reading mode if the button is pressed for a long time
  // Reading mode allows us to just keep reading data from TOF
  bool readingMode = isButtonLongPressed();
  delay(1000);
  // Give time to the user to place the sensor in the correct position
  waitToAddTof();
  if (!readingMode) {
    // Try to calibrate the sensor
    Serial.println("Starting ToF Calibration...");
    displayMessage("CALIBRATE", 5, 2);
    bool calibSuccess = false;
#ifdef DEBUG
    calibSuccess = lox.calibrate(0x29, true);
#else
    calibSuccess = lox.calibrate(0x29, false);
#endif
  
    // For some reason, we have to re-initialize the display after calibration
    if(!display.begin(SSD1306_SWITCHCAPVCC)) {
      Serial.println(F("Failed to init display"));
      while(1); // Don't proceed, loop forever
    }

    if (calibSuccess) {
      Serial.println("Calibration Done");
      displayMessage("OK!", 30, 4, true);

      // Programmatically reboot the sensor
      digitalWrite(TOF_POWER, LOW);
      delay(1000);
      waitToRemoveTof();
      digitalWrite(TOF_POWER, HIGH);
      delay(1000);
      waitToAddTof();
      // Make sure average reading from sensor after
      // calibration is within threshold
      if (initTof()) {
        displayMessage("READING", 25, 2);
        if (isAverageReadingOkay()) {
          Serial.println(F("Calibration SUCCESS"));
          displayMessage("OK!", 30, 4, true, true);
        } else {
          Serial.println(F("Average readings out of range"));
          displayMessage("FAILED", 30, 2, true, true);
        }
      }
    } else {
      Serial.println("Calibration Failed");
      displayMessage("FAILED", 30, 2, true, true);
    }
  } else {
    if (initTof()) {
      displayMessage("READING", 25, 2);
      // Start reading values from the TOF for eternity
      // unless we get some error from the sensor
      while (true) {
        int range = getReadingFromTof();
        if (isButtonPressed()) {
          buttonPressed = true;
          Serial.println("Stopping reading mode...");
          break;
        }
        if (range < 0) {
          break;
        }
        // Have a delay between consecutive readings
        delay(100);
      }
    }
  }
}

bool initTof() {
#ifdef DEBUG
  if (lox.begin(0x29, true))
#else
  if (lox.begin(0x29, false))
#endif
  {
    return true;
  }
  Serial.println(F("Failed to boot VL53L0X"));
  return false;
}

void displayMessage(String msg, uint8_t startX, uint8_t textSize, bool invert, bool addDelay) {
  display.clearDisplay();

  display.setTextSize(textSize); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(startX, 25);
  display.println(msg);
  display.invertDisplay(invert);
  display.display();      // Show initial text
  if (addDelay) {
    delay(2000);
  }
}

bool scanForTof() {
  byte error, address; //variable for error and I2C address
#ifdef DEBUG
  Serial.println("Scanning for sensor...");
#endif
  for (address = 1; address < 127; address++ ) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
#ifdef DEBUG
      Serial.print("Sensor found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
#endif
      return true;
    }
#ifdef DEBUG
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
#endif
  }

  return false;
}

void waitToAddTof() {
  while (!scanForTof()) {
    delay(2000);
  }
}

void waitToRemoveTof() {
   while (scanForTof()) {
    delay(2000);
  }
}

bool isAverageReadingOkay() {
  uint32_t acc = 0;
  uint8_t cnt = 0;
  // Take average of max 50 readings
  for (int i = 0; i < 50; i++) {
    // Accumulate reading for average
    int range = getReadingFromTof();
    if (range != -1) {
      acc += range;
      ++cnt;
    }
    // Have a delay between consecutive readings
    delay(100);
  }
  return lox.isAverageReadingOkay(acc/cnt);
}

int getReadingFromTof() {
    VL53L0X_RangingMeasurementData_t measure;
#ifdef DEBUG
  Serial.print("Reading a measurement... ");
#endif
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
  
  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    uint16_t range = measure.RangeMilliMeter;
    if (range > MAX_RANGE) {
      displayMessage("ERROR!", 20, 3, true, true);
#ifdef DEBUG
      printError(range);
#endif
    } else {
#ifdef DEBUG
      Serial.print("Distance (mm): "); Serial.println(range);
#endif
      return range;
    }
  } else {
#ifdef DEBUG
    Serial.println(" out of range ");
#endif
  }
  return -1;
}

bool isButtonPressed() {
  button.loop();
  return button.isPressed();
}

bool isButtonReleased() {
  button.loop();
  return button.isReleased();
}

bool isButtonLongPressed() {
  unsigned long startTime = 0, endTime = 0;
  bool isPressing = false;
  while (true) {
    button.loop();
    // Store time when we start pressing the button
    if (button.isPressed()) {
      startTime = millis();
      isPressing = true;
    }
    // If we releaase the button, check whether it was a
    // long press or a short press
    if (button.isReleased()) {
      endTime = millis();
      isPressing = false;
      return (endTime - startTime) >= LONG_PRESS_TIME;
    }
    // Check if we have kept pressing the button for longer than LONG_PRESS_TIME
    if (isPressing && millis() - startTime >= LONG_PRESS_TIME) {
      isPressing = false;
      return true;
    }
  }
}

void printError(uint16_t error) {
  switch(error) {
    case ERROR_HARDWARE_FAIL:
      Serial.println("ERROR_HARDWARE_FAIL: Either VCSEL or VHV failure detected");
      break;
    case ERROR_PHASE_FAIL:
      Serial.println("ERROR_PHASE_FAIL: Noise too high in signal");
      break;
    case ERROR_MIN_RANGE_FAIL:
      Serial.println("ERROR_MIN_RANGE_FAIL: Reading below minimum range of the sensor");
      break;
    case ERROR_SIGNAL_FAIL:
      Serial.println("ERROR_SIGNAL_FAIL: Return signal too low to give accurate reading");
      break;
    case ERROR_SIGMA_FAIL:
      Serial.println("ERROR_SIGMA_FAIL: Too much noise due to ambient light");
      break;
    default:
      Serial.println("READING OUT OF RANGE!");
      break;
  }
}
