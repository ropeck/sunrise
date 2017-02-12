#include "InternetButton.h"

byte nonsense_var = 0;  //sacrifice to the ifdef 

#define ON "1"
#define OFF "0"

#define MUTE_WEMO 1

#define DEBUG 1

#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif

InternetButton b = InternetButton();
TCPClient client;
String swaddr[] = {"10.0.1.10", "10.0.1.18"};
// living room 10.0.1.10
// bedroom 10.0.1.18

int wemoPort = 49153;

int xValue;
int yValue;
int zValue;

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

void switchSet(String state, String host) {
  TCPClient client;
  String data1;

  DEBUG_PRINT("switchState "+state+" "+host);
#ifdef MUTE_WEMO
  DEBUG_PRINT("muted");
  return;
#endif

  data1 += "<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>";
  data1 += state;
  data1 += "%d</BinaryState></u:SetBinaryState></s:Body></s:Envelope>";

  if (client.connect(host,wemoPort)) {
        client.println("POST /upnp/control/basicevent1 HTTP/1.1");
        client.println("Content-Type: text/xml; charset=utf-8");
        client.println("SOAPACTION: \"urn:Belkin:service:basicevent:1#SetBinaryState\"");
        client.println("Connection: keep-alive");
        client.print("Content-Length: ");
        client.println(data1.length());
        client.println();
        client.print(data1);
        client.println();
    }

  if (client.connected()) {
     client.stop();
  }
}

void switchOn() {
  for (int i=0; i< 2; i++) {
    switchSet(ON, swaddr[i]);
  }
}


void switchOff() {
  for (int i=0; i< 2; i++) {
    switchSet(OFF, swaddr[i]);
  }
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





