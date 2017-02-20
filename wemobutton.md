# Notes for Library to Support Wemo Button
  In addition to controlling the wemo devices, the button can show up
as a device on the wemo remote app and visible to the Google Home.
  Adding a few lines to the sunrise alarm code will let it be a remote
controlled light.

## TODO

  * add the web client code
  * actions
    * turn on
    * turn off
    * status
  * set the client name to the name of the photon device
    * where does this come from?

https://docs.particle.io/reference/firmware/core/#get-device-name
```

// Open a serial terminal and see the device name printed out
void handler(const char *topic, const char *data) {
    Serial.println("received " + String(topic) + ": " + String(data));
}

void setup() {
    Serial.begin(115200);
    for(int i=0;i<5;i++) {
        Serial.println("waiting... " + String(5 - i));
        delay(1000);
    }

    Particle.subscribe("spark/", handler);
    Particle.publish("spark/device/name");
}

```
