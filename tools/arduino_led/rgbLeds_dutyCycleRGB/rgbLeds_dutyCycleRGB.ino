/*
Modulate LED based on input from serial communication
*/

#include <stdio.h>

// Each RGB LED counts as three LEDs
const int numLeds = 3;
const int ledPins[numLeds] = {31, 33, 35};

const int modulationPeriodMicroseconds = 2000;

// percentBrightness = modulationOnMicroseconds / modulationPeriodMicroseconds
//const int modulationOnMicroseconds[numLeds] = {125, 30, 30}; 
//const int modulationOnMicroseconds[numLeds] = {10, 10, 10};
const int modulationOnMicroseconds[numLeds] = {125, 60, 100};

const int microsecondsPerFrame = 33333;
const int microsecondPause = microsecondsPerFrame/2; // TODO: pick a good value
const int totalPeriodFrames = 15;
const int totalPeriodMicroseconds = totalPeriodFrames * microsecondsPerFrame; //500000;

int whichLeds[3] = {0, 1, 2};
int numOnFrames[3] = {5, 5, 2};

// For bHood style car LEDs
#define LIGHT_ON LOW
#define LIGHT_OFF HIGH

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
    digitalWrite(ledPins[iLed], LIGHT_OFF);
  }
} // void setup()

void updateTimings()
{
  if(Serial.available()) {
    const int bufferLength = 64;
    char buffer[bufferLength];

    int local_whichLeds[3] = {-1, -1, -1};
    int local_numOnFrames[3] = {-1, -1, -1};

    delay(100);

    int i;
    for(i=0; i<bufferLength-1; i++) {
      if(!Serial.available())
        break;

      const int byteRead = Serial.read();

      buffer[i] = byteRead;
    }
    buffer[i] = '\0';

    //Serial.println(buffer);

    // Example: 0,5,1,5,2,2
    sscanf(buffer, "%d,%d,%d,%d,%d,%d", &local_whichLeds[0], &local_numOnFrames[0], &local_whichLeds[1], &local_numOnFrames[1], &local_whichLeds[2], &local_numOnFrames[2]);

    bool isValid = true;

    int totalOnFrames = 0;

    for(int i=0; i<3; i++) {
      totalOnFrames += local_numOnFrames[i];
      if(local_whichLeds[i] < 0 || local_whichLeds[i] > 2 || local_numOnFrames[i] < 0 || whichLeds[i] < 0 || whichLeds[i] > 2) {
        isValid = false;
        break;
      }
    }
    
    if(totalOnFrames >= totalPeriodFrames) {
      isValid = false;
    }
    
    snprintf(buffer, bufferLength, "%d,%d %d,%d %d,%d", local_whichLeds[0], local_numOnFrames[0], local_whichLeds[1], local_numOnFrames[1], local_whichLeds[2], local_numOnFrames[2]);
    Serial.println(buffer);

    if(!isValid) {
      Serial.println("Invalid input. Use format: whichLed0, numOnFrames0, whichLed1, numOnFrames1, whichLed2, numOnFrames2");
      return;
    }

    // If we received too many bytes, just throw the rest out
    while(Serial.available()) {
      Serial.read();
    }
    
    for(int i=0; i<3; i++) {
      whichLeds[i] = local_whichLeds[i];
      numOnFrames[i] = local_numOnFrames[i];
    }
  } // if(Serial.available()) {
} // void updateTimings()

static inline void flashLed(const int whichLedPin, const int numMicrosecondsOn) 
{
  digitalWrite(whichLedPin, LIGHT_ON);
  bool ledIsOn = true;

  // Inner loop, modulating on and off very quickly, to form a brightness
  // For each LED, wait for the appropriate amount of time, then turn it off
  const unsigned long innerStartTime = micros();
  while(true) {
    const unsigned long curTime = micros();
    const unsigned long deltaTime = curTime - innerStartTime;

    if(deltaTime >= modulationPeriodMicroseconds) {
      break;
    }

    if(ledIsOn && (deltaTime > numMicrosecondsOn)) {
      digitalWrite(whichLedPin, LIGHT_OFF);
      ledIsOn = false;
    }
  } // while(true)

  if(ledIsOn) {
    digitalWrite(whichLedPin, LIGHT_OFF);
    ledIsOn = false;
  }
}

// the loop function runs over and over again forever
void loop()
{
  const unsigned long mainStartTime = micros();
  
  for(int iLed=0; iLed<3; iLed++) {
    const unsigned long ledLoopStartTime = micros();
    
    const int whichLed = whichLeds[iLed];
    const int curLedPin = ledPins[whichLed];
    
    int numOnMicroseconds;
    if(iLed == 2) {
      numOnMicroseconds = numOnFrames[iLed] * microsecondsPerFrame - microsecondPause;
    } else {
      numOnMicroseconds = numOnFrames[iLed] * microsecondsPerFrame;
    }
    
    bool ledIsOn = true;
  
    // Outer loop, for "On" vs Off
    unsigned long outerStartTime = micros();
    unsigned long mainLoopTimeDelta = outerStartTime - ledLoopStartTime;
    while(mainLoopTimeDelta <= numOnMicroseconds) {
      
      if(ledIsOn) {
        flashLed(curLedPin, modulationOnMicroseconds[whichLed]);
      }
  
      outerStartTime = micros();
      mainLoopTimeDelta = outerStartTime - ledLoopStartTime;
    } // while(true)
    
    digitalWrite(curLedPin, LIGHT_OFF);
  } // for(int iLed=0; iLed<3; iLed++)

  // Finish with dark
  unsigned long outerStartTime = micros();
  unsigned long mainLoopTimeDelta = outerStartTime - mainStartTime;
  while(mainLoopTimeDelta < totalPeriodMicroseconds) {
    outerStartTime = micros();
    mainLoopTimeDelta = outerStartTime - mainStartTime;
  } // while((mainLoopTime - loopStartTime) < totalPeriodMicroseconds)

  updateTimings();
} // void loop()

