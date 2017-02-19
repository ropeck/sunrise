#include "InternetButton.h"

byte nonsense_var = 0;  //sacrifice to the ifdef 

#define ON "1"
#define OFF "0"

#ifdef DEBUG
 #define DEBUG_PRINT(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
#endif
#define TRICK17(x) x

char hostbuf[255];
char *bufcur;
typedef struct {
  char *name;
  char *addr;
  int port;
  TCPClient client;
} WemoDev;
WemoDev device[16];
int devcount;

TRICK17(char *fetchHttp(WemoDev *));
void _resetWemoDeviceList() {
  bufcur = hostbuf;
  devcount = 0;
}

void _updateWemoDevice(char *url) {
  WemoDev *w;
  char *portstr;
  Serial.println(url);
  strcpy(bufcur, url);
  w = &device[devcount];
  w->addr = bufcur;
  portstr = strchr(w->addr,':');
  *portstr++ = 0;
  bufcur = portstr;
  w->port = atoi(portstr); 
  w->name = bufcur;
  *bufcur = 0;
  fetchHttp(w);
  
  devcount++;
}

#define DATABUFSIZE 10240
char datahttp[DATABUFSIZE];
TRICK17(char *fetchHttp(WemoDev *w)) {
  char buf[128];
// LOCATION: http://10.0.1.15:49153/setup.xml
  if (w->client.connect(w->addr,w->port)) {
        w->client.print("GET http://");
        w->client.print(String(w->addr));
        sprintf(buf,":%i/setup.xml HTTP/1.1", w->port);
        w->client.println(buf);
        w->client.println();
   }
}

TRICK17(void switchSet(String state, WemoDev *w)) {
  TCPClient client;
  String data1;
  char buf[255];

  // only do rooms ... that is, things with 'oom' in the name

  sprintf(buf, "set %s %s:%d %s", state.c_str(), w->addr, w->port, w->name);  
  DEBUG_PRINT(buf);

  if (strstr(w->name, "oom") == 0) {
    DEBUG_PRINT("not a room. skipping state.");
    return;
  }
#ifdef MUTE_WEMO
  DEBUG_PRINT("muted");
  return;
#endif

  data1 += "<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>";
  data1 += state;
  data1 += "</BinaryState></u:SetBinaryState></s:Body></s:Envelope>";

  if (client.connect(w->addr,w->port)) {
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

void _switch(String state) {
  for (int i=0; i< devcount; i++) {
    switchSet(state, &device[i]);
  }
}

void switchOn() {
  _switch(ON);
}

void switchOff() {
  _switch(OFF);
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
  TCPClient client;

  if (lastTime == 0 || millis() - lastTime > 30 * 1000) {
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

  char buf[512];
  char *name;
  // check all the devices names
  for (int i=0; i<devcount; i++) {
   WemoDev *w = &device[i];
   client = device[i].client; 
     if (client.available()) {
       client.readString().toCharArray(datahttp, DATABUFSIZE);
// <friendlyName>Living Room</friendlyName>
       char *name = strstr(datahttp, "<friendlyName>") + 14;
       char *e = strstr(datahttp, "</friendlyName>");
       *e = 0;
       w->name = bufcur;
       strcpy(bufcur, name);
       bufcur += strlen(name);
       *bufcur++ = 0;
       sprintf(buf, "WemoDev %i %s:%i %s",
   	       devcount, w->addr, w->port, w->name);
       Serial.println(buf);
       if (client.connected()) {
         client.stop();
       }
     }
  }

}

