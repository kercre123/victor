/*
Modulate LED based on input from serial communication
*/

#include <stdio.h>

// Each RGB LED counts as three LEDs
const int numLeds = 3;
const int centerLedPins[numLeds] = {31, 33, 35};
const int headlightsPin = 37;

const int modulationPeriodMicroseconds = 2000;

// percentBrightness = modulationOnMicroseconds / modulationPeriodMicroseconds
//const int modulationOnMicroseconds[numLeds] = {125, 30, 30}; 
//const int modulationOnMicroseconds[numLeds] = {10, 10, 10};
const int modulationOnMicroseconds[numLeds] = {125, 60, 100};

const int microsecondsPerFrame = 33333;
const int microsecondPause = microsecondsPerFrame/2; // TODO: pick a good value
const int totalPeriodFrames = 15;
const int totalPeriodMicroseconds = totalPeriodFrames * microsecondsPerFrame; //500000;
const int framesOff = 3;

// int whichLeds[3] = {0, 1, 2};
// int numOnFrames[3] = {5, 5, 2};
int whichLeds[3] = {0, 2, 1};
int numOnFrames[3] = {8, 4, 0};

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
    pinMode(centerLedPins[iLed], OUTPUT);
    digitalWrite(centerLedPins[iLed], LIGHT_OFF);
  }
} // void setup()

void updateTimings()
{
  if(Serial.available()) {
    const int bufferLength = 64;
    char buffer[bufferLength];

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


    bool isValid = true;
    bool isRed;
    if(buffer[0] == 'r' || buffer[0] == 'R') {
      isRed = true;
    } else if(buffer[0] == 'b' || buffer[0] == 'B') {
      isRed = false;
    } else {
      isValid = false;
    } 
    
    int numFramesFirst = -1;
    if(isValid) {
      numFramesFirst = strtol(buffer+1, NULL, 10);
      //sscanf(buffer+1, "%d", &local_whichLeds[0], &local_numOnFrames[0], &local_whichLeds[1], &local_numOnFrames[1], &local_whichLeds[2], &local_numOnFrames[2]);
  
      if(numFramesFirst < 1 || numFramesFirst > (totalPeriodFrames-1-framesOff)) {
        isValid = false;
      }
    }
    
    Serial.println(buffer);
    
    //snprintf(buffer, bufferLength, "%c %d", buffer[0], isRed);
    //Serial.println(buffer);

    if(!isValid) {
      Serial.println("Invalid input. Use format: R# or B#");
      return;
    }

    // If we received too many bytes, just throw the rest out
    while(Serial.available()) {
      Serial.read();
    }
    
    if(isRed) {
      whichLeds[0] = 0;
      whichLeds[1] = 2;
      whichLeds[2] = 1;
      numOnFrames[0] = numFramesFirst;
      numOnFrames[1] = totalPeriodFrames - framesOff - numFramesFirst;
      numOnFrames[2] = 0;
    } else {
      whichLeds[0] = 2;
      whichLeds[1] = 0;
      whichLeds[2] = 1;
      numOnFrames[0] = numFramesFirst;
      numOnFrames[1] = totalPeriodFrames - framesOff - numFramesFirst;
      numOnFrames[2] = 0;
    }
  } // if(Serial.available()) {
} // void updateTimings()

static inline void flashLed(const int whichCenterLedPin, const int whichHeadlightsPin, const int numMicrosecondsOn) 
{
  digitalWrite(whichCenterLedPin, LIGHT_ON);
  
  if(whichHeadlightsPin > 0) {
    pinMode(whichHeadlightsPin, OUTPUT);
    digitalWrite(whichHeadlightsPin, HIGH);
  }
  
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
      digitalWrite(whichCenterLedPin, LIGHT_OFF);
      if(whichHeadlightsPin > 0) {
        pinMode(whichHeadlightsPin, INPUT);
      }
      ledIsOn = false;
    }
  } // while(true)

  if(ledIsOn) {
    digitalWrite(whichCenterLedPin, LIGHT_OFF);
    if(whichHeadlightsPin > 0) {
      pinMode(whichHeadlightsPin, INPUT);
    }
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
    const int curCenterLedPin = centerLedPins[whichLed];
    const int curHeadlightsLedPin = (iLed==0) ? headlightsPin : -1;
    
    int numOnMicroseconds;
    if(iLed == 2) {
      if(numOnFrames[iLed] * microsecondsPerFrame < microsecondPause) {
        numOnMicroseconds = 0;
      } else {
        numOnMicroseconds = numOnFrames[iLed] * microsecondsPerFrame - microsecondPause;
      }
    } else {
      numOnMicroseconds = numOnFrames[iLed] * microsecondsPerFrame;
    }
      
    bool ledIsOn = true;
  
    // Outer loop, for "On" vs Off
    unsigned long outerStartTime = micros();
    unsigned long mainLoopTimeDelta = outerStartTime - ledLoopStartTime;
    while(mainLoopTimeDelta <= numOnMicroseconds) {
      
      if(ledIsOn) {
        flashLed(curCenterLedPin, curHeadlightsLedPin, modulationOnMicroseconds[whichLed]);
      }
  
      outerStartTime = micros();
      mainLoopTimeDelta = outerStartTime - ledLoopStartTime;
    } // while(true)
    
    digitalWrite(curCenterLedPin, LIGHT_OFF);
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

