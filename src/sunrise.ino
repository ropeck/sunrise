#include "InternetButton.h"

#include "Time.h"
#include "TimeLib.h"

time_t alarm = 0;
tmElements_t tm;

#define ON "1"
#define OFF "0"


#define ASLEEP 0
#define AWAKE 1

//#define MUTE_WEMO 1
//#undef DEBUG

#ifdef DEBUG
  char debug_buf2[255];

#ifndef DEBUG_PRINT
 #define DEBUG_PRINT(fmt, ...) sprintf(debug_buf2, fmt, __VA_ARGS__); Serial.println(debug_buf2);
#else
 #define DEBUG_PRINT(fmt, ...)
#endif
#endif

#define TZ_OFFSET -8*60*60

time_t time_now() {
  return Time.now() + TZ_OFFSET
}

int xValue;
int yValue;
int zValue;

InternetButton b = InternetButton();
TCPClient client;

void switchOn();
void switchOff();
void loopWemo(InternetButton);
void setupWemo(InternetButton);


int toggle_state(int s) {
  if (s == AWAKE) {
    return ASLEEP;
  } else {
    return AWAKE;
  }
}

// https://github.com/neilbartlett/color-temperature/blob/master/index.js

char *timeStr(time_t t) {
  static char tbuf[32];
  sprintf(tbuf, "%d/%d %d:%02d:%02d", month(t), day(t),
	  hour(t), minute(t), second(t));
  return tbuf;
}

int state;
time_t alarm_time[2]; // ASLEEP:6am  AWAKE:10pm

int brightness(time_t t, time_t alarm) {
  time_t prealarm;
  time_t now = time_now();
  prealarm = alarm - 30*60; // 30 minutes earlier
   
  int n = 255 * (int)(now - prealarm) / (30*60);
  n = min(max(n,0),255);
  if (time_now() % 10 == 0) {
    char alarmStr[16];
    strncpy(alarmStr, timeStr(alarm), 16);
    DEBUG_PRINT("t %s %s color %d", timeStr(now), alarmStr, n);
  }
  return n;
}

void setColor() {
  int n = brightness(time_now(), alarm);
  b.allLedsOn(n,n,0);
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

void beep(int state) { 
  switch (state) {
    case ASLEEP:
     // switchOff();
      b.playSong("E5,8,G5,8,E6,8,C6,8,D6,8,G6,8,");
      break;
    case AWAKE:
      switchOn();
     // b.playSong("C4,8,E4,8,G4,8,C5,8,G5,4,");
      break;
  }
  flashlights();
}

int debug_mode = 0;

int debugmode(String command) {
  int mode = atoi(command.c_str());
  b.playSong("G4,8,G4,8,G4,8,");
  debug_mode = mode;
  if (mode == 1) {
    b.playSong("C5,4,");
  } else {
    b.playSong("C2,4,");
  }
}

TCPServer server = TCPServer(80);

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

  IPAddress myIP = WiFi.localIP();
  Serial.print("my address http://");
  Serial.println(myIP);
  server.begin();

  state = AWAKE;
  setAlarms();
}


void setAlarms() {
  tmElements_t tm;
  breakTime(time_now(), tm);
  tm.Hour = 6;
  tm.Minute = 0;
  tm.Second = 0;
  alarm_time[ASLEEP]= makeTime(tm);  // 6am
  if (alarm_time[ASLEEP] < time_now()) {
    alarm_time[ASLEEP] += 60*60*24;
  }
  tm.Hour = 22;
  alarm_time[AWAKE]= makeTime(tm);   // 10pm
  if (alarm_time[AWAKE] < time_now()) {
    alarm_time[AWAKE] += 60*60*24;
  }
}

time_t nextTime = 0;

void showDevices();

void loopWeb() {
  if (client.connected()) {
    char buf[25555];
    char *i = buf;
    int size;
    while ((size = client.available()) > 0) {
      uint8_t buf[size];
      client.read(buf, size);
      Serial.write(buf, size);
    }
    Serial.println("");
    client.stop();
  } else {
    client = server.available();
  }
}


void loop() {
  if (time_now() > alarm) {
    setAlarms();
  }
  alarm = alarm_time[state];
  
  setColor();  
  if (anyButtonPressed()) {
    state = toggle_state(state);
    beep(state);
  }
  // if prev_color != color: adjust the color one step closer
  if (time_now() >= nextTime) {
    showDevices();
    DEBUG_PRINT("time: %s", timeStr(time_now()));
    DEBUG_PRINT("alarm: %s", timeStr(alarm));
    nextTime = time_now() + 10;
  }
  loopWemo(b);
  loopWeb();
  delay(50);
}


