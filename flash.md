# How To Flash Particle
  Updating the code on the particle is simple.  You use the particle
cli to flash the code.

## Example


```

Rodneys-iMac:sunrise fogcat5$ pwd
/Users/fogcat5/sunrise



Rodneys-iMac:sunrise fogcat5$ particle list
loose [270021000b47353137323334] (Photon) is offline
two [2c0035000f51353338363333] (Photon) is offline
Bedroom [250034000447333437333039] (Photon) is online



Rodneys-iMac:sunrise fogcat5$ particle flash Bedroom
Including:
    src/Time.h
    src/TimeLib.h
    src/sunrise.ino
    src/wemo.ino
    src/Time.cpp
    project.properties
attempting to flash firmware to your device Bedroom
Flash device OK:  Update started

Rodneys-iMac:sunrise fogcat5$ git remote -v
origin	git@github.com:ropeck/sunrise.git (fetch)
origin	git@github.com:ropeck/sunrise.git (push)

```


