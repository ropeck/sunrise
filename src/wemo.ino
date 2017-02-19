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
char *bufcur;
struct WemoDev {
  char *name;
  char *addr;
  int port;
};
struct WemoDev device[16];
int devcount;

void _resetWemoDeviceList() {
  bufcur = hostbuf;
  devcount = 0;
}

void _updateWemoDevice(char *url) {
  struct WemoDev *w;
  char buf[512];
  char *name;
  char *portstr;
  strcpy(bufcur, url);
  w = &device[devcount];
  w->addr = bufcur;
  portstr = strchr(w->addr,':');
  *portstr++ = 0;
  bufcur += strlen(url);
  *bufcur++ = 0;
  w->port = atoi(portstr); 
  w->name = bufcur;
  name = fetchHttp(w->addr, w->port);
  strcpy(bufcur, name);
  bufcur += strlen(name);
  *bufcur = 0;
  
  sprintf(buf, "WemoDev %i %s:%i %s",
	  devcount, w->addr, w->port, w->name);
  Serial.println(buf);

  devcount++;
}

#define DATABUFSIZE 10240
char datahttp[DATABUFSIZE];
char *fetchHttp(char *host, int port) {
  TCPClient client;
  char buf[128];
// LOCATION: http://10.0.1.15:49153/setup.xml

    if (client.connect(host,port)) {
        client.print("GET http://");
        client.print(String(host));
        sprintf(buf,":%i/setup.xml HTTP/1.1", port);
        client.println(buf);
        client.println();
    }
  *datahttp = 0; 
  while (client.connected()) {
    while (*datahttp == 0 || client.available()) {
      client.readString().toCharArray(datahttp, DATABUFSIZE);
    }
  }
// <friendlyName>Living Room</friendlyName>
  char *name = strstr(datahttp, "<friendlyName>") + 14;
  char *e = strstr(datahttp, "</friendlyName>");
  *e = 0;
  return name;
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

  randomSeed(analogRead(A0));
  delay(random(2000));  // avoid herd load on power up
  udp.begin(1901);      // prep to listen for wemo devices
}

unsigned long lastTime = 0;
void loopWemo() {
  char *host;
  int packetSize;
  IPAddress UpNPIP(239, 255, 255, 250);
  int UpNPPort = 1900;

  if (millis() - lastTime > 30 * 1000) {
      lastTime = millis();
      _resetWemoDeviceList(); 
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
      Serial.println("Scanning for devices");
      //Serial.println("Sending:");
      //Serial.print(searchPacket);
      //Serial.println();
      udp.beginPacket(UpNPIP, UpNPPort);
      udp.write(searchPacket);
      udp.endPacket();
  }
  
        packetSize = udp.parsePacket();
        while(packetSize != 0){
            //Serial.println("Device discovered");
            byte packetBuffer[packetSize+1];
            udp.read(packetBuffer, packetSize);
            host = strstr((char *)packetBuffer, "LOCATION: http://")+17;
            char *end = strstr(host, "/");
            *end = 0;
	    _updateWemoDevice(host);
            packetSize = udp.parsePacket();
        }	    
}

