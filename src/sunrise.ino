#include "InternetButton.h"

#define ON "1"
#define OFF "0"

#define MUTE_WEMO 1


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

void loop() {
// have to ignore button 4 because it's also the D7 led and I want that off
    
    if (!(digitalRead(4) && digitalRead(5) && digitalRead(6))) {
      switchOff();
      flashlights();
      return;
    }
    int shake = (abs((xValue-b.readX())+abs(yValue-b.readY())+
		 abs(zValue-b.readZ())));
    DEBUG_PRINT("shake" + shake);

    if (shake > 100) {
      switchOn();
      flashlights();
    }

    xValue = b.readX();
    yValue = b.readY();
    zValue = b.readZ();
    delay(50);  // a little delay to help measure movement
}
