/*
Modulate LED based on input from serial communication
*/

#include <stdio.h>

const int numLeds = 2;

const int ledPins[numLeds][3] = {{31, 33, 35}, {37, 39, 41}};

const int modulationPeriodMicroseconds = 1000;
const int brightnessPeriodMicroseconds = 10000000;
const int inverseMax = 16; // 2 is half, 3 is 1/3, etc

//const int deltaModulationPeriodMicroseconds = 5;
unsigned long startTime = 0;

void setDefaultTimings()
{
  startTime = 0;
  /*
  for(int iLed=0; iLed<numLeds; iLed++) {
    for(int iChannel=0; iChannel<3; iChannel++) {
      ledOnMicroseconds[iLed][iChannel] = 1000;
    }
  }*/
} // void setDefaultTimings()

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(57600);
 
  setDefaultTimings();
  
  for(int iLed=0; iLed<numLeds; iLed++) {
    for(int iChannel=0; iChannel<3; iChannel++) {
      pinMode(ledPins[iLed][iChannel], OUTPUT);
    }
  }
} // void setup() 

void updateTimings()
{
  /*if(Serial.available()) {
    const int bufferLength = 1024;
    char buffer[bufferLength];
    
    int local_modulationPeriodMicroseconds;
    int local_brightnessPeriodMicroseconds;
    
    delay(250);
    
    int i;
    for(i=0; i<bufferLength-1; i++) {
      if(!Serial.available())
        break;
     
      const int byteRead = Serial.read();
      
      buffer[i] = byteRead;
      
      //Serial.print("%d ", buffer[i]);
      //Serial.print(byteRead, DEC);
    }
    buffer[i] = '\0';
    //Serial.print("\n");
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      local_microsecondsOn[iLed] = -1;
      local_microsecondsOff[iLed] = -1;
    }
    
    sscanf(buffer, "%d,%d,%d,%d,%d,%d", &local_microsecondsOn[0], &local_microsecondsOff[0], &local_microsecondsOn[1], &local_microsecondsOff[1], &local_microsecondsOn[2], &local_microsecondsOff[2]);
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      if(local_microsecondsOn[iLed] < 0 || local_microsecondsOn[iLed] > 10000000 || local_microsecondsOff[iLed] < 0 || local_microsecondsOff[iLed] > 10000000) {
        Serial.println("Invalid input. Use format: led0TimeOn, led0TimeOff, led1TimeOn, led1TimeOff, led2TimeOn, led2TimeOff");
        return;
      }
    }
    
    // If we received too many bytes, just throw the rest out
    while(Serial.available()) {
      Serial.read();
    }
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      ledIsOn[iLed] = false;
      nextLedSwitchTime[iLed] = 0;
      microsecondsOn[iLed] = local_microsecondsOn[iLed];
      microsecondsOff[iLed] = local_microsecondsOff[iLed];
      
      Serial.print(iLed, DEC);
      Serial.print(". ");
      Serial.print(microsecondsOn[iLed], DEC);
      Serial.print(" ");
      Serial.print(microsecondsOff[iLed], DEC);
      Serial.print("\n");
    }
  } // if(Serial.available()) {
    */
} // void updateTimings()

// the loop function runs over and over again forever
void loop()
{
  unsigned long ledOnMicroseconds[numLeds][3];
  
  const unsigned long loopStartTime = micros();
  const unsigned long timeSinceStart = loopStartTime-startTime;
  
  // If it's been a full second (or the value of brightnessPeriodMicroseconds), reset the brightnesses
  if(timeSinceStart > brightnessPeriodMicroseconds) {
    startTime = loopStartTime;
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      for(int iChannel=0; iChannel<3; iChannel++) {
        ledOnMicroseconds[iLed][iChannel] = 0;
      }
    }
  } else {
    const int inversePeriod = brightnessPeriodMicroseconds / modulationPeriodMicroseconds;
    const int curOnMicroseconds = timeSinceStart / inversePeriod / inverseMax;
    // TODO: adjust brightness
    for(int iLed=0; iLed<numLeds; iLed++) {
      for(int iChannel=0; iChannel<3; iChannel++) {
        ledOnMicroseconds[iLed][iChannel] = curOnMicroseconds;
        //ledOnMicroseconds[iLed][iChannel] = 0;
      }
    }
       
    //ledOnMicroseconds[0][1] = modulationPeriodMicroseconds / inverseMax;
       
    //Serial.print(curOnMicroseconds, DEC);
    //Serial.println();
  }
  
  bool ledIsOn[numLeds][3];
  
  // Turn all lights on
  for(int iLed=0; iLed<numLeds; iLed++) {
    for(int iChannel=0; iChannel<3; iChannel++) {
      digitalWrite(ledPins[iLed][iChannel], HIGH);
      //digitalWrite(ledPins[iLed][iChannel], LOW);
      ledIsOn[iLed][iChannel] = true;
    }
  }
  
  // For each LED, wait for the appropriate amount of time, then turn it off
  const unsigned long whileStartTime = micros();
  while(true) {
    const unsigned long curTime = micros();
    const unsigned long deltaTime = curTime - whileStartTime;
    
    if(deltaTime >= modulationPeriodMicroseconds) {
      break;
    }
    
    //Serial.print(ledOnMicroseconds[0][2], DEC);
    //Serial.println();
    
    for(int iLed=0; iLed<numLeds; iLed++) {
      for(int iChannel=0; iChannel<3; iChannel++) {
        if(ledIsOn[iLed][iChannel] && (deltaTime > ledOnMicroseconds[iLed][iChannel])) {
          digitalWrite(ledPins[iLed][iChannel], LOW);
          ledIsOn[iLed][iChannel] = false;
        }
      }
    }
  } // while(true)
  
  /*for(int iLed=0; iLed<numLeds; iLed++) {
    for(int iChannel=0; iChannel<3; iChannel++) {
      if(ledIsOn[iLed][iChannel]) {
        digitalWrite(ledPins[iLed][iChannel], LOW);
      }
    }
  }*/
  
  //updateTimings();
} // void loop() 
