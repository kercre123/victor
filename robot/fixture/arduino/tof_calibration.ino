#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_VL53L0X.h"
#include "vl53l0x_api_calibration.h"

#define CALIBTRATION_THRESH 10

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_DC     8
#define OLED_CS     10
#define OLED_RESET  9

// Add function definitions
void displayMessage(String msg, uint8_t startX, uint8_t textSize, bool addDelay = false);
bool scanForTof(void);

// Declare global variables
bool calibSuccess = false;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
  Wire.begin();
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  Serial.println("Calibrator started");
  displayMessage("TOF-CALIB", 10, 2, true);
}


  
void loop() {
  // Wait for sensor to be plugged-in
  while (!scanForTof())
  {
    displayMessage("READY", 30, 2);
    delay(10000);
  }
  // Try to calibrate the sensor
  Serial.println("Starting ToF Calibration...");
  displayMessage("CALIBRATE", 5, 2, true);
  if(!lox.calibrate())
  {
    calibSuccess = false;
    displayMessage("FAILED", 10, 2);
    Serial.println("Calibration Failed");
  }
  else // Perform ranging to check if the sensor is calibrated correctly
  {
    uint16_t acc = 0;
    uint8_t cnt = 0;
    for (int i = 0; i < 20; i++)
    {
      VL53L0X_RangingMeasurementData_t measure;

      Serial.print("Reading a measurement... ");
      lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
    
      if (measure.RangeStatus != 4) {  // phase failures have incorrect data
        Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
        acc += measure.RangeMilliMeter;
        cnt++;
      } else {
        Serial.println(" out of range ");
      }
        
      delay(100);
    }
    if (lox.isAverageReadingOkay(acc/cnt))
    {
      calibSuccess = true;
      Serial.println("Calibration SUCCECSS");
    }
    else
    {
      calibSuccess = false;
      Serial.println("Average readings out of range");
    }
  }

  // Wait for sensor to be removed
  while(scanForTof())
  {
    if (calibSuccess)
    {
      displayMessage("OK!", 30, 4);
    }
    else
    {
      displayMessage("FAILED", 10, 2);
    }
    delay(10000);
  }
  Serial.println("Sensor Removed");
  // Make this false for calibration of next sensor
  calibSuccess = false;
}

void displayMessage(String msg, uint8_t startX, uint8_t textSize, bool addDelay) {
  display.clearDisplay();

  display.setTextSize(textSize); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(startX, 25);
  display.println(msg);
  display.display();      // Show initial text
  if (addDelay)
  {
    delay(2000);
  }
}

bool scanForTof()
{
  byte error, address; //variable for error and I2C address

  Serial.println("Scanning for sensor...");

  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("Sensor found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      return true;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  return false;
}
