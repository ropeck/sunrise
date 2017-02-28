#include "InternetButton.h"

#include "Time.h"
#include "TimeLib.h"

time_t alarm;
tmElements_t tm;

#define ON "1"
#define OFF "0"

#define ASLEEP 0
#define AWAKE 1

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
void loopWemo(InternetButton);
void setupWemo(InternetButton);


int toggle_state(s) {
  if (s == AWAKE) {
    return ASLEEP;
  } else {
    return AWAKE;
  }
}

// https://github.com/neilbartlett/color-temperature/blob/master/index.js

char *timeStr(time_t t) {
  static char tbuf[32];
  sprintf(tbuf, "%d:%02d:%02d", hour(t), minute(t), second(t));
  return tbuf;
}

void setColor() {    // set led colors for current time of day
  time_t prealarm;
  time_t alarm;
  time_t now = Time.now() - 8*60*60;
    breakTime(now, tm);  // adjust for PDT and UTC
    DEBUG_PRINT("time now %s", timeStr(now));
    tm.Hour = 6;
    tm.Minute = 0;
    tm.Second = 0;
    alarm = makeTime(tm);
    if (Time.now() > alarm) {
      alarm += 24*60*60;
    }
    prealarm = alarm - 30*60; // 30 minutes earlier
    DEBUG_PRINT("alarm %s", timeStr(alarm));
  time_t t = now;

  t = min(max(prealarm, t),alarm);
   
  int n = 255 * (t - prealarm) / (30*60);

  if (t > alarm + 15*60) {
    n = 0;
  }

  b.allLedsOn(n,n,0);
  DEBUG_PRINT("color %d", n);
}

int state;
time_t alarm_time[2]; // ASLEEP:6am  AWAKE:10pm

void setup() {
    b.begin();
   
    RGB.control(true);
    RGB.brightness(32);  
    pinMode(D7, INPUT_PULLDOWN);
    Serial.begin(9600);
    xValue = b.readX();
    yValue = b.readY();
    zValue = b.readZ();
    setupWemo(b);
    state = AWAKE;
    alarm_time[ASLEEP]=0; // TODO
    alarm_time[AWAKE]=0;
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

void beep(int state) { 
  switch (state) {
    case ASLEEP:
      switchOff();
      b.playSong("E5,8,G5,8,E6,8,C6,8,D6,8,G6,8");
      break;
    case AWAKE:
      switchOn();
      b.playSong("C4,8,E4,8,G4,8,C5,8,G5,4");
      break;
  }
  flashlights();
}


time_t nextTime = 0;

void showDevices();
void loop() {
  time_t = alarm_times[state];
  setColor();  
  if anyButtonPressed() {
    state = toggle_state(state);
    beep(state);
    //set color = 0
  }
  // if prev_color != color: adjust the color one step closer
}

void oldloop() {
    if (Time.now() >= nextTime) {
      showDevices();
      setColor();  
      // DEBUG_PRINT("time: %d", (int)Time.now());
      nextTime = Time.now() + 10;
      Serial.println("");
    }
    loopWemo(b);
// have to ignore button 4 because it's also the D7 led and I want that off
       delay(50);  // a little delay to help measure movement
}
