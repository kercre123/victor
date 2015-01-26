/*
Modulate LED based on input from serial communication
*/

#include <stdio.h>

// Each RGB LED counts as three LEDs
const int numLeds = 3;
const int ledPins[numLeds] = {31, 33, 35};

const int modulationPeriodMicroseconds = 2000;
const int modulationOnMicroseconds[numLeds] = {250, 60, 60}; // percentBrightness = modulationOnMicroseconds / modulationPeriodMicroseconds

const int microsecondsPerFrame = 33333;
const int microsecondPause = microsecondsPerFrame/2; // TODO: pick a good value
const int totalPeriodFrames = 15;
const int totalPeriodMicroseconds = totalPeriodFrames * microsecondsPerFrame; //500000;

int whichLed = 0;
int numOnFrames = 6;

void setDefaultTimings()
{
} // void setDefaultTimings()

// the setup function runs once when you press reset or power the board
void setup()
{
  Serial.begin(57600);

  setDefaultTimings();

  for(int iLed=0; iLed<numLeds; iLed++) {
    pinMode(ledPins[iLed], OUTPUT);
    digitalWrite(ledPins[iLed], LOW);
  }
} // void setup()

void updateTimings()
{
  if(Serial.available()) {
    const int bufferLength = 32;
    char buffer[bufferLength];

    int local_whichLed = -1;
    int local_numOnFrames = -1;

    delay(25);

    int i;
    for(i=0; i<bufferLength-1; i++) {
      if(!Serial.available())
        break;

      const int byteRead = Serial.read();

      buffer[i] = byteRead;
    }
    buffer[i] = '\0';

    //Serial.println(buffer);

    // Example: 0,5
    sscanf(buffer, "%d,%d", &local_whichLed, &local_numOnFrames);

    bool isValid = true;

    if(local_whichLed < 0 || local_whichLed > 2 || local_numOnFrames > totalPeriodFrames || local_numOnFrames < 0) {
        isValid = false;
    }

    Serial.print(local_whichLed, DEC);
    Serial.print(", ");
    Serial.println(local_numOnFrames, DEC);

    if(!isValid) {
      Serial.println("Invalid input. Use format: whichLed, numOnFrames");
      return;
    }

    // If we received too many bytes, just throw the rest out
    while(Serial.available()) {
      Serial.read();
    }

    whichLed = local_whichLed;
    numOnFrames = local_numOnFrames;
  } // if(Serial.available()) {
} // void updateTimings()

// the loop function runs over and over again forever
void loop()
{
  const unsigned long mainStartTime = micros();

  const int curLedPin = ledPins[whichLed];
  const int numOnMicroseconds = numOnFrames * microsecondsPerFrame - microsecondPause;
  bool ledIsOn = true;

  unsigned long outerStartTime = micros();

  // Outer loop, for "On" vs Off
  while((outerStartTime - mainStartTime) < totalPeriodMicroseconds) {
    const unsigned long mainLoopTimeDelta = outerStartTime - mainStartTime;

    if(mainLoopTimeDelta > numOnMicroseconds) {
      digitalWrite(curLedPin, LOW);
      ledIsOn = false;
      outerStartTime = micros();
      continue;
    }
    
    digitalWrite(curLedPin, HIGH);
    ledIsOn = true;
    
    // Inner loop, modulating on and off very quickly, to form a brightness
    // For each LED, wait for the appropriate amount of time, then turn it off
    const unsigned long innerStartTime = micros();
    while(true) {
      const unsigned long curTime = micros();
      const unsigned long deltaTime = curTime - innerStartTime;

      if(deltaTime >= modulationPeriodMicroseconds) {
        break;
      }

      if(ledIsOn && (deltaTime > modulationOnMicroseconds[whichLed])) {
        digitalWrite(curLedPin, LOW);
        ledIsOn = false;
      }
    } // while(true)

    if(ledIsOn) {
      digitalWrite(curLedPin, LOW);
      ledIsOn = false;
    }

    outerStartTime = micros();
  } // while((mainLoopTime - loopStartTime) < totalPeriodMicroseconds)

  updateTimings();
} // void loop()

