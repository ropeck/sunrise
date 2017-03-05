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

#define TZ_OFFSET -8*60*60

#ifdef REALTIME
time_t time_now() {
  return Time.now() + TZ_OFFSET;
}
#endif

time_t makeTimeAt(int hour) {
  tmElements_t tm;
  breakTime(Time.now(), tm);
  tm.Hour = hour;
  tm.Minute = 0;
  tm.Second = 0;
  return makeTime(tm);
}

time_t time_startup = Time.now();

#ifndef REALTIME
time_t test_time_start = makeTimeAt(5)+60*50;
int test_time_multiple = 10;

time_t time_now() {
  return (Time.now() - time_startup) * test_time_multiple + test_time_start;
}
#endif

char *timeStr(time_t t) {
  static char tbuf[32];
  sprintf(tbuf, "%d/%d %d:%02d:%02d", month(t), day(t),
	  hour(t), minute(t), second(t));
  return tbuf;
}

int state;
time_t alarm_time[2]; // ASLEEP:6am  AWAKE:10pm

int brightness(time_t t, time_t alarm) {
  char *statestr;
  static time_t next_time;
  time_t prealarm;
  time_t now = time_now();
  prealarm = alarm - 30*60; // 30 minutes earlier
   
  int n = 255 * (int)(now - prealarm) / (30*60);
  n = min(max(n,0),255);
  if (time_now() > next_time) {
    next_time = time_now() + 10;
    char alarmStr[16];
    strncpy(alarmStr, timeStr(alarm), 16);
    statestr = "ASLEEP";
    if (state == AWAKE) {
      statestr = "AWAKE";
    }
    DEBUG_PRINT("%s %s %s color %d", statestr, timeStr(now), alarmStr, n);
  }
  return n;
}

// http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/

void setColorTemp(int temp, int brightness) {
  float cr, cg, cb;
  float t = temp / 100;

  cr = 255;
  if (t > 66) {
    cr = t - 60;
    cr = min(255, 329.698727446 * pow(cr, -0.1332047592));
  }

  if (t <= 66) {
    cg = 99.4708025861 * log(t) - 161.1195681661;
  } else {
    cg = t - 60;
    cg = 288.1221695283 * pow(cg, -0.0755149402);
  }    
  cg = min(255, max(0, cg));

  cb = 255;
  if (t < 66) {
    if (t <= 19) {
      cb = 0;
    } else {
      cb = t - 10;
      cb = 138.5177312231 * log(cb) - 305.0447827307;
    }
  }
  cb = min(255, max(0, cb));

  DEBUG_PRINT("%d %d %d %d %d", temp, brightness, int(cr), int(cg), int(cb));

  b.allLedsOn(int(cr*brightness/255),int(cg*brightness/255),int(cb*brightness/255));
}

// vary the temp from 2000 to 6500K to go from red to white
void setColor() {
  setColorTemp(2000, brightness(time_now(), alarm));
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

void debugmode(String command) {
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
  alarm_time[ASLEEP]= makeTimeAt(6);  // 6am
  alarm_time[AWAKE]= makeTimeAt(22);   // 10pm
  if (time_now() > alarm_time[AWAKE] + 60*60) {
    alarm_time[AWAKE] += 60*60*24;
  } 
  if (time_now() > alarm_time[ASLEEP] + 60*60) {
    alarm_time[ASLEEP] += 60*60*24;
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
    setAlarms();
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
}


