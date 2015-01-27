/*
Modulate LED based on input from serial communication
*/

#include <stdio.h>

const int numLeds = 3;

const int ledPins[numLeds] = {22, 24, 26};

int microsecondsOn[numLeds];
int microsecondsOff[numLeds];

bool ledIsOn[numLeds];
unsigned long nextLedSwitchTime[numLeds];

void setDefaultTimings()
{
  for(int iLed=0; iLed<numLeds; iLed++) {
    microsecondsOn[iLed] = 1000000;
    microsecondsOff[iLed] = 1000000;
    ledIsOn[iLed] = false;
    nextLedSwitchTime[iLed] = 0;
  }
} // void setDefaultTimings()

// the setup function runs once when you press reset or power the board
void setup() 
{
  Serial.begin(9600);
 
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
    
    int local_microsecondsOn[numLeds];
    int local_microsecondsOff[numLeds];
    
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
} // void updateTimings()

// the loop function runs over and over again forever
void loop()
{
  const unsigned long curTime = micros();
  
  for(int iLed=0; iLed<numLeds; iLed++) {
    if(curTime > nextLedSwitchTime[iLed]) {
      if(ledIsOn[iLed]) {
        digitalWrite(ledPins[iLed], HIGH);
        nextLedSwitchTime[iLed] = curTime + microsecondsOff[iLed];
      } else {
        digitalWrite(ledPins[iLed], LOW);
        nextLedSwitchTime[iLed] = curTime + microsecondsOn[iLed];
      }
      
      ledIsOn[iLed] = !ledIsOn[iLed];
    } // if(curTime > nextLedSwitchTime[iLed])
  } // for(int iLed=0; iLed<numLeds; iLed++)
  
  /*
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  //Serial.println("Off");
  delay(1000);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  //Serial.println("On");
  delay(1000);*/
  
  updateTimings();
} // void loop() 
