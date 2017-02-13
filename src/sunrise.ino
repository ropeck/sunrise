#include "InternetButton.h"

#define ON "1"
#define OFF "0"

// #define MUTE_WEMO 1
//#undef DEBUG

#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif


int xValue;
int yValue;
int zValue;

InternetButton b = InternetButton();
TCPClient client;

void switchOn();
void switchOff();

void setup() {
    b.begin();
    RGB.control(true);
    RGB.brightness(32);  
    pinMode(D7, INPUT_PULLDOWN);
    Serial.begin(9600);
    xValue = b.readX();
    yValue = b.readY();
    zValue = b.readZ();
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
    char buf[255];

    xValue = b.readX();
    yValue = b.readY();
    zValue = b.readZ();
    sprintf(buf, "shake %d", shake);
    DEBUG_PRINT(buf);
    return (shake > 100);
}

void loop() {
// have to ignore button 4 because it's also the D7 led and I want that off
    
    if (anyButtonPressed()) {
      switchOff();
      b.playSong("C4,8,E4,8,G4,8,C5,8,G5,4");
      flashlights();
      return;
    }

    if (shaken()) {
      switchOn();
      b.playSong("E5,8,G5,8,E6,8,C6,8,D6,8,G6,8");
      flashlights();
    }

    delay(50);  // a little delay to help measure movement
}
