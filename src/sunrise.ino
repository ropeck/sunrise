#include "InternetButton.h"

#include "Time.h"
#include "TimeLib.h"

time_t alarm;
tmElements_t tm;

#define ON "1"
#define OFF "0"

// #define MUTE_WEMO 1
//#undef DEBUG


#ifdef DEBUG
  char debug_buf2[255];

#ifndef DEBUG_PRINT
 #define DEBUG_PRINT(fmt, ...) sprintf(debug_buf2, fmt, __VA_ARGS__); Serial.println(debug_buf2);
#else
 #define DEBUG_PRINT(fmt, ...)
#endif
#endif


int xValue;
int yValue;
int zValue;

InternetButton b = InternetButton();
TCPClient client;

void switchOn();
void switchOff();
void loopWemo();
void setupWemo();

enum states {
  ALARM_SET,
  ALARM_OFF,
  ALARM_RINGING
};
enum states state;

// https://github.com/neilbartlett/color-temperature/blob/master/index.js

void setColor() {    // set led colors for current time of day
  time_t prealarm;
    breakTime(Time.now(), tm); 
    DEBUG_PRINT("time now %02d:%02d", tm.Hour, tm.Minute);
    tm.Hour = 14;  // 6am Pacific
    tm.Hour = (14+12+4)%24;  // 10pm Pacific
    tm.Minute = 0;
    tm.Second = 0;
    alarm = makeTime(tm);
    prealarm = alarm - 30*60; // 30 minutes earlier
    if (Time.now() > alarm) {
      alarm += 24*60*60;
      prealarm += 24*60*60;
    }
    DEBUG_PRINT("time %d", (int)Time.now());
    DEBUG_PRINT("alarm %d", (int)alarm);
  time_t t = Time.now();

  t = min(max(prealarm, t),alarm);
  
  int n = 255 * (t - prealarm) / (30*60);
  b.allLedsOn(n,n,0);
  DEBUG_PRINT("color %d", n);
}

void setup() {
    b.begin();
   
    RGB.control(true);
    RGB.brightness(32);  
    pinMode(D7, INPUT_PULLDOWN);
    Serial.begin(9600);
    xValue = b.readX();
    yValue = b.readY();
    zValue = b.readZ();
    setupWemo();
    state = ALARM_SET;
}

void flashlights() {
      b.rainbow(10);
      delay(100);
      b.allLedsOff();
}

boolean anyButtonPressed() {
    return (!(digitalRead(4) && digitalRead(5) && digitalRead(6)));
}

int shaken() {
    int shake = (abs((xValue-b.readX())+abs(yValue-b.readY())+
		 abs(zValue-b.readZ())));
    //char buf[255];

    xValue = b.readX();
    yValue = b.readY();
    zValue = b.readZ();
    //sprintf(buf, "shake %d", shake);
    //DEBUG_PRINT(buf);
    return (shake > 100);
}


int brightness(int when) {
  // 0 to 255 over the 30 minutes before the alarm time
  // pick a time for now as the target
  static int br = 0;
  int diff = when - Time.now();
  br = 0;
  if (diff < 0) {
    br = 255;
  }
  if (diff < 255 && diff > 0) {
    br = 255 - diff;
  }

  // DEBUG_PRINT("br %d %d %d", br, when, Time.now());
  return br;
}

time_t nextTime = 0;

void loop() {
    loopWemo();
    if (Time.now() > nextTime) {
      setColor();  
      DEBUG_PRINT("time: %d", (int)Time.now());
      nextTime = Time.now() + 10;
    }
// have to ignore button 4 because it's also the D7 led and I want that off
    
    if (anyButtonPressed()) {
      switchOff();
      b.playSong("E5,8,G5,8,E6,8,C6,8,D6,8,G6,8");
      flashlights();
      return;
    }

    if (shaken()) {
      switchOn();
      b.playSong("C4,8,E4,8,G4,8,C5,8,G5,4");
      flashlights();
    }

    delay(50);  // a little delay to help measure movement
}
