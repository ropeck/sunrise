#include "InternetButton.h"

byte nonsense_var = 0;  //sacrifice to the ifdef 

#define ON "1"
#define OFF "0"

#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif

String swaddr[] = {"10.0.1.7", "10.0.1.3"};
// living room 10.0.1.7
// bedroom 10.0.1.3

int wemoPort = 49153;

char subnet[16];  // subnet prefix of all devices
char hostbuf[255];
char *bufcur = hostbuf;
struct WemoDev {
  char *name;
  char *addr;
};
struct WemoDev device[16];
int devcount = 0;

void _updateWemoDevice(char *url) {
  struct WemoDev *w;
  char buf[255];
  w = &device[devcount];
  w->addr = bufcur;
  strcpy(bufcur, url);
  bufcur += strlen(url);

  sprintf(buf, "WemoDev %i ", devcount);
  Serial.print(buf);
  Serial.println(device[devcount].addr);

  devcount++;
}

char *fetchHttp(char *url) {
  TCPClient client;
  String host="10.0.1.7";

  if (client.connect(host,wemoPort)) {
        client.print("GET ");
        client.print(String(url));
        client.print(" HTTP/1.1");
        client.println();
    }
  while (!client.available()) { delay(500); }
  Serial.println(String(client.read()));

  if (client.connected()) {
     client.stop();
  }
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

UDP udp;
void setupWemo() {
  // scan for wemo switches with a udp broadcast


  IPAddress UpNPIP(239, 255, 255, 250);
  int UpNPPort = 1900;

    String searchPacket = "M-SEARCH * HTTP/1.1\r\n";
    searchPacket.concat("HOST: 239.255.255.250:1900\r\n");
    searchPacket.concat("MAN: \"ssdp:discover\"\r\n");
    searchPacket.concat("MX: ");
    searchPacket.concat("5");
    searchPacket.concat("\r\n");
    searchPacket.concat("ST: ");
    //searchPacket.concat("urn:Belkin:device:controllee:1");
    searchPacket.concat("urn:Belkin:device:1");
    searchPacket.concat("\r\n");
    searchPacket.concat("\r\n");
    Serial.println("Sending:");
    Serial.print(searchPacket);
    Serial.println();
    udp.begin(1901);
    udp.beginPacket(UpNPIP, UpNPPort);
    udp.write(searchPacket);
    udp.endPacket();
}

unsigned long lastTime = 0;
void loopWemo() {
  char *url;

  if (lastTime == 0) {
      lastTime = millis();
  }
  if (millis() - lastTime > 1000) {
        int packetSize = 0;
        packetSize = udp.parsePacket();
        while(packetSize != 0){
            Serial.println("Device discovered");
            byte packetBuffer[packetSize+1];
            udp.read(packetBuffer, packetSize);
            url = strstr((char *)packetBuffer, "LOCATION: ")+10;
            char *end = strstr(url, "xml")+3;
            *end = 0;
	    _updateWemoDevice(url);
	    
            packetSize = udp.parsePacket();
        }
// get the location
// LOCATION: http://10.0.1.15:49153/setup.xml

      lastTime = millis();
  }
}



