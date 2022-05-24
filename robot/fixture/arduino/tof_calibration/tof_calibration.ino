#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_VL53L0X.h"
#include "vl53l0x_api_calibration.h"
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
// Uncomment line below to view debug statements
// #define DEBUG

// Function definitions
void displayMessage(String msg, uint8_t startX, uint8_t textSize, bool invert = false, bool addDelay = false);
bool scanForTof(void);
void waitToAddTof(bool useButton = false);
void waitToRemoveTof(void);
bool isAverageReadingOkay(void);
bool isButtonPressed(void);

// Global variables
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
ezButton button(BUTTON);

// Boot up init things
void setup() {
  Wire.begin();
  Serial.begin(115200);
  pinMode(TOF_POWER, OUTPUT);
  digitalWrite(TOF_POWER, HIGH);

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
  // Wait for sensor to be plugged-in
  displayMessage("READY", 30, 2);
  waitToAddTof(true);
  // Give time to the user to place the sensor in the correct position

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

  if (calibSuccess)
  {
    Serial.println("Calibration Done");
    displayMessage("OK!", 30, 4, true);
  }
  else
  {
    Serial.println("Calibration Failed");
    displayMessage("FAILED", 30, 2, true, true);
  }

  // Perform some ranging measurements
  if (calibSuccess)
  {
    // Programmatically reboot the sensor
    digitalWrite(TOF_POWER, LOW);
    delay(1000);
    waitToRemoveTof();
    digitalWrite(TOF_POWER, HIGH);
    delay(1000);
    waitToAddTof();

#ifdef DEBUG
    if (lox.begin(0x29, true))
#else
    if (lox.begin(0x29, false))
#endif
    {
      if (isAverageReadingOkay())
      {
        Serial.println(F("Calibration SUCCESS"));
        displayMessage("OK!", 30, 4, true, true);
      }
      else
      {
        Serial.println(F("Average readings out of range"));
        displayMessage("FAILED", 30, 2, true, true);
      }
    }
    else
    {
      Serial.println(F("Failed to boot VL53L0X"));
    }
  }
}

void displayMessage(String msg, uint8_t startX, uint8_t textSize, bool invert, bool addDelay) {
  display.clearDisplay();

  display.setTextSize(textSize); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(startX, 25);
  display.println(msg);
  display.invertDisplay(invert);
  display.display();      // Show initial text
  if (addDelay)
  {
    delay(2000);
  }
}

bool scanForTof()
{
  byte error, address; //variable for error and I2C address
#ifdef DEBUG
  Serial.println("Scanning for sensor...");
#endif
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
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
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
#endif
  }

  return false;
}

void waitToAddTof(bool useButton)
{
  if (useButton)
  {
    while (!isButtonPressed())
    {
      delay(1);
    }
  }
  while (!scanForTof())
  {
    delay(2000);
  }
}

void waitToRemoveTof()
{
   while (scanForTof()) {
    delay(2000);
  }
}

bool isAverageReadingOkay()
{
  uint16_t acc = 0;
  uint8_t cnt = 0;
  displayMessage("READING", 25, 2);
  for (int i = 0; i < 50; i++)
  {
    VL53L0X_RangingMeasurementData_t measure;
#ifdef DEBUG
    Serial.print("Reading a measurement... ");
#endif
    lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
    
    if (measure.RangeStatus != 4) {  // phase failures have incorrect data
#ifdef DEBUG
      Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
#endif
      acc += measure.RangeMilliMeter;
      cnt++;
    } else {
#ifdef DEBUG
      Serial.println(" out of range ");
#endif
    }
    delay(100);
   }
  if (lox.isAverageReadingOkay(acc/cnt))
  {
    return true;
  }
  return false;
}

bool isButtonPressed()
{
  button.loop();
  return button.isReleased();
}
