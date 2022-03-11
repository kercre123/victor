#include <SPI.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_VL53L0X.h"
#include "vl53l0x_api_calibration.h"

// OLED parameters
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_DC     8
#define OLED_CS     10
#define OLED_RESET  9

// Uncomment this to view debug statements
//#define DEBUG

// Add function definitions
void displayMessage(String msg, uint8_t startX, uint8_t textSize, bool addDelay = false);
bool scanForTof(void);

// Declare global variables
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
  Wire.begin();
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    while(1); // Don't proceed, loop forever
  }
  display.clearDisplay();
  Serial.println("Calibrator started");
  displayMessage("TOF-CALIB", 10, 2, true);

  // Wait for sensor to be plugged-in
  while (!scanForTof())
  {
    displayMessage("READY", 30, 2);
    delay(2000);
  }
  // Try to calibrate the sensor
  Serial.println("Starting ToF Calibration...");
  displayMessage("CALIBRATE", 5, 2);
  // Allow some time for the user to plug in the sensor
  // delay(5000);
  bool calibSuccess = false;
#ifdef DEBUG  
  if(!lox.calibrate(0x29, true))
  {
    Serial.println("Calibration Failed");
  }
#else
  if(!lox.calibrate())
  {
    Serial.println("Calibration Failed");
  }
#endif  
  else // Perform ranging to check if the sensor is calibrated correctly
  {
    uint16_t acc = 0;
    uint8_t cnt = 0;
    for (int i = 0; i < 20; i++)
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
      calibSuccess = true;
      Serial.println("Calibration SUCCECSS");
    }
    else
    {
      Serial.println("Average readings out of range");
    }
  }

  // For some reason, wew have to re-initialize the display after calibration
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    while(1); // Don't proceed, loop forever
  }
  if (calibSuccess)
  {
    displayMessage("OK!", 30, 4, true);
  }
  else
  {
    displayMessage("FAILED", 30, 2, true);
  }
}


  
void loop() {
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
