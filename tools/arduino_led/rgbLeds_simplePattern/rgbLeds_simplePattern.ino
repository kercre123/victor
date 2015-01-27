/*
Modulate LED based on input from serial communication
*/

#include <stdio.h>

// Each RGB LED counts as three LEDs
const int numLeds = 3;

//const int ledPins[numLeds] = {31, 33, 35, 37, 39, 41};
const int ledPins[numLeds] = {31, 33, 35};

int modulationPeriodMicroseconds;
int modulationOnMicroseconds[numLeds]; // percentBrightness = modulationOnMicroseconds / modulationPeriodMicroseconds

int totalPeriodMicroseconds;
int ledOnMicroseconds[numLeds]; 

void setDefaultTimings()
{ 
  modulationPeriodMicroseconds = 1000;
  totalPeriodMicroseconds = 1000000;

  for(int iLed=0; iLed<numLeds; iLed++) {
    ledOnMicroseconds[iLed] = totalPeriodMicroseconds / 2;
    modulationOnMicroseconds[iLed] = 60;
  }
  
  // Approximately red
  modulationOnMicroseconds[0] = 250;
} // void setDefaultTimings()

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(19200);
 
  setDefaultTimings();
  
  for(int iLed=0; iLed<numLeds; iLed++) {
    pinMode(ledPins[iLed], OUTPUT);
  }
} // void setup() 

void updateTimings()
{
  if(Serial.available()) {
    const int bufferLength = 1024;
    char buffer[bufferLength];
    
    int local_modulationPeriodMicroseconds = -1;
    int local_modulationOnMicroseconds[numLeds];
    int local_totalPeriodMicroseconds = -1;
    int local_ledOnMicroseconds[numLeds];
    
    delay(250);
    
    int i;
    for(i=0; i<bufferLength-1; i++) {
      if(!Serial.available())
        break;
     
      const int byteRead = Serial.read();
      
      buffer[i] = byteRead;
    }
    buffer[i] = '\0';
    
    //Serial.println(buffer);
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      local_modulationOnMicroseconds[iLed] = -1;
      local_ledOnMicroseconds[iLed] = -1;
    }
    
    // Example for approximately even RGB for one LED: 1000000,1000,250,1000000,60,1000000,60,1000000
    sscanf(buffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &local_totalPeriodMicroseconds, &local_modulationPeriodMicroseconds, &local_modulationOnMicroseconds[0], &local_ledOnMicroseconds[0], &local_modulationOnMicroseconds[1], &local_ledOnMicroseconds[1], &local_modulationOnMicroseconds[2], &local_ledOnMicroseconds[2], &local_modulationOnMicroseconds[3], &local_ledOnMicroseconds[3], &local_modulationOnMicroseconds[4], &local_ledOnMicroseconds[4], &local_modulationOnMicroseconds[5], &local_ledOnMicroseconds[5]);
    
    bool isValid = true;
    
    if(local_modulationPeriodMicroseconds < 0 || local_modulationPeriodMicroseconds > 100000 || local_modulationPeriodMicroseconds > totalPeriodMicroseconds || local_totalPeriodMicroseconds < 0 || local_totalPeriodMicroseconds > 10000000) {
        isValid = false;
    }
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      if(local_ledOnMicroseconds[iLed] < 0 || local_ledOnMicroseconds[iLed] > totalPeriodMicroseconds || local_modulationOnMicroseconds[iLed] < 0 || local_modulationOnMicroseconds[iLed] > local_modulationPeriodMicroseconds) {
          isValid = false;
        break;
      }
    }
    
    Serial.print("(");
    Serial.print(local_totalPeriodMicroseconds, DEC);
    Serial.print(", ");
    Serial.print(local_modulationPeriodMicroseconds, DEC);
    Serial.print(") ");
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      modulationOnMicroseconds[iLed] = local_modulationOnMicroseconds[iLed];
      ledOnMicroseconds[iLed] = local_ledOnMicroseconds[iLed];
      
      Serial.print(iLed, DEC);
      Serial.print(":");
      Serial.print(local_modulationOnMicroseconds[iLed], DEC);
      Serial.print(",");
      Serial.print(local_ledOnMicroseconds[iLed], DEC);
      Serial.print("  ");
    }
    Serial.print("\n");
    
    if(!isValid) {
      Serial.println("Invalid input. Use format: totalPeriodMicroseconds, modulationPeriodMicroseconds, modulationOnMicroseconds[0], ledOnMicroseconds[0], ..., modulationOnMicroseconds[5], ledOnMicroseconds[5]");
      return;
    }

    // If we received too many bytes, just throw the rest out
    while(Serial.available()) {
      Serial.read();
    }
    
    modulationPeriodMicroseconds = local_modulationPeriodMicroseconds;
    totalPeriodMicroseconds = local_totalPeriodMicroseconds;
  } // if(Serial.available()) {
} // void updateTimings()

// the loop function runs over and over again forever
void loop()
{
  const unsigned long mainStartTime = micros();
  
  bool ledIsOn[numLeds];
  
  unsigned long outerStartTime = micros();

  // Outer loop, for "On" vs Off
  while((outerStartTime - mainStartTime) < totalPeriodMicroseconds) {
    const unsigned long mainLoopTimeDelta = outerStartTime - mainStartTime;
    
    // Turn all lights on that should be on
    for(int iLed=0; iLed<numLeds; iLed++) {
      if(mainLoopTimeDelta < ledOnMicroseconds[iLed]) {
        digitalWrite(ledPins[iLed], HIGH);
        ledIsOn[iLed] = true;
      }
    }
    
    // Inner loop, modulating on and off very quickly, to form a brightness
    // For each LED, wait for the appropriate amount of time, then turn it off
    const unsigned long innerStartTime = micros();
    while(true) {
      const unsigned long curTime = micros();
      const unsigned long deltaTime = curTime - innerStartTime;
      
      if(deltaTime >= modulationPeriodMicroseconds) {
        break;
      }
      
      for(int iLed=0; iLed<numLeds; iLed++) {
        if(ledIsOn[iLed] && (deltaTime > modulationOnMicroseconds[iLed])) {
          digitalWrite(ledPins[iLed], LOW);
          ledIsOn[iLed] = false;
        }
      }
    } // while(true)
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      if(ledIsOn[iLed]) {
        digitalWrite(ledPins[iLed], LOW);
        ledIsOn[iLed] = false;
      }
    }
    
    outerStartTime = micros();
  } // while((mainLoopTime - loopStartTime) < totalPeriodMicroseconds)
  
  updateTimings();
} // void loop() 
