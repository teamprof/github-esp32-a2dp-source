## ESP32 A2DP Source controller via I2C
This project uses an ESP32 as a Bluetooth A2DP source to play audio on a Bluetooth earphone/speaker. A master MCU (e.g., Coral Dev Board Micro) sends commands to the ESP32 via the I2C bus. The ESP32 then plays audio on an earphone/speaker via A2DP.

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](https://github.com/teamprof/esp32-a2dp-source/blob/main/LICENSE)

<a href="https://www.buymeacoffee.com/teamprof" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 38px !important;width: 128px !important;" ></a>

---
## Hardware
The following components are required for this project:
1. ESP32 Dev Kit V1 
2. Bluetooth earphone/speaker
3. Coral Dev Board Micro
4. (optional) coral-esp32-adapter
[![coral-esp32-adapter](/doc/image/pcb.jpg)](https://github.com/teamprof/esp32-a2dp-source/blob/main/doc/image/pcb.jpg)
5. Computer with 2 USB ports
## *** Hardware information will be available in E/Nov ***

---
### System Block diagram
```
    A.I. device                             wearable device
    
    +--------+                           +--------------------+
    | camera |                           |   swimmming cap    | (the cap prevent lost of earphone during swimming)
    +--------+ +-------+    Bluetooth    +--------------------+
    | coral  |-| ESP32 |     (A2DP)      | Bluetooth earphone |
    +--------+ +-------+                 | (bone conduction)  |
                                         +--------------------+

```
The firmware of this project runs on the ESP32 in the A.I. device portion above.

For more information, pleasae refer to https://www.hackster.io/contests/buildtogether/hardware_applications/15719


### System image 
## *** System image will be available in E/Nov ***

---
## Software setup
1. Install [Arduino IDE 2.0+ for Arduino](https://www.arduino.cc/en/Main/Software)
2. Install [A Simple Arduino Bluetooth Music Receiver and Sender for the ESP32](https://github.com/pschatzmann/ESP32-A2DP)
3. Install [Arduino DebugLog lib](https://www.arduino.cc/reference/en/libraries/debuglog/)
4. Download and extract this esp32-a2dp-source code from github 
---


## Sound output
Sound will be played based on the following situations.
```
   +--------------------------------------+---------------------+
   | Situation                            |    sound file       |
   +--------------------------------------+---------------------+
   | swimmer nears the edge of the pool   | soundEdgePool.mp3   |
   |                                      |                     |
   | swimmer nears left lane              | soundLeftLane.mp3   |
   | swimmer at the middle of lane        | soundMiddleLane.mp3 |
   | swimmer nears right lane             | soundRightLane.mp3  |
   |                                      |                     |
   | error                                |     error.mp3       |
   +--------------------------------------+---------------------+
```
note:
* sound chips is contributed by Pixabay (https://pixabay.com/)
* sound is cropped to 0.1 seconds


Example of sound output:
```
when the swimmer is near the edge of the pool and at the middle of lane, soundEdgePool and soundMiddleLane will be heared by the swimmer, as below
       0.1s           0.1s            0.1s            0.1s            0.1s
|---------------|---------------|---------------|---------------|---------------|
                  soundEdgePool                                  soundMiddleLane

when the swimmer is near the edge of the pool and nears left/right lane, soundEdgePool and soundLeftLane/soundRightLane will be heared by the swimmer, as below
       0.1s           0.1s            0.1s            0.1s            0.1s
|---------------|---------------|---------------|---------------|---------------|
                  soundEdgePool                                  soundLeftLane /
                                                                 soundRightLane 
```


## Hardware connection between Coral Dev Board Micro and ESP32
```
+------------------------+     +---------------------+
| Coral Dev Board Micro  |     |       ESP32         |
+-------+----------------+     +------+--------------+
| GPIO  |   description  |     | GPIO |  description |
+-------+----------------+     +------+--------------+
| kSCL1 |   I2C1 SCL     |     |  22  |   I2C SCL    |
| kSDA1 |   I2C1 SDA     |     |  21  |   I2C SDA    |
+-------+----------------+     +------+--------------+
```

## I2C communication format
```
 command: IsA2dpConnected
                  +----------------+----------------+
                  |     byte 0     |     byte 1     |
                  +----------------+----------------+
 master to slave  |IsA2dpConnected |       0        |
                  +----------------+----------------+
 slave to master  |       0        |     result     |
                  +----------------+----------------+


command: PlaySound
                 +----------------+----------------+----------------+----------------+
                 |     byte 0     |     byte 1     |     byte 2     |     byte 3     |
                 +----------------+----------------+----------------+----------------+
master to slave  |   PlaySound    |     volume     |     sound      |        0       |
                 +----------------+----------------+----------------+----------------+
slave to master  |       0        |        0       |       0        |     result     |
                 +----------------+----------------+----------------+----------------+

```

## Flow of communication between Coral Dev Board Micro and ESP32
```
    coral                                 esp32
      |                                     |
      |   I2cCommand::IsA2dpConnected       |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::A2dpConnected or     |
      |   I2cResponse::A2dpDisconnected     |
      | <---------------------------------- |
      |                                     |
      |                                     |
      |       loop IsA2dpConnected          |
      |       until A2dpConnected           |
      |                                     |
      |                                     |
      |   I2cCommand::PlaySound             |
      |   param: <volumne> <sound>          |
      | ----------------------------------> |
      |                                     |
      |   I2cResponse::Success or           |
      |   I2cResponse::ErrorInvalidParam or |
      |   I2cResponse::ErrorDisconnected    |
      | <---------------------------------- |

    note:
    1. coral sends I2cCommand::PlaySound command every 0.5s
    2. coral re-start I2cCommand::IsA2dpConnected flow once "I2cResponse::ErrorDisconnected" is received
```


## Code explanation

### Connect to Bluetooth earphone/speaker
Once the ESP32 boots up, it searches for and connects to a Bluetooth earphone/speaker by name.
The name of the Bluetooth earphone/speaker is defined by the "TARGET_DEVICE_NAME" macro in "./src/thread/ThreadApp.cpp"
Here's an example of a Bluetooth speaker with the name "BTS-06".
```
#define TARGET_DEVICE_NAME "BTS-06"      // bluetooth name of earphone/speaker
```

### I2C address
The I2C slave address is defined in the macro "I2C_DEV_ADDR" in the file "./src/thread/ThreadApp.cpp"
Here's an example of a I2C slave address as "0x55".
```
#define I2C_DEV_ADDR ((uint8_t)0x55)
```

## *** source code will be available in E/Nov ***

---

## Demo
## *** Video demo will be available in E/Nov ***


---
### Debug
Enable or disable log be modifying macro on "AppLog.h"

Debug is disabled by "#define DEBUGLOG_DISABLE_LOG"
Enable trace debug by "#define DEBUGLOG_DEFAULT_LOG_LEVEL_TRACE"

Example of AppLog.h
```
// Disable Logging Macro (Release Mode)
// #define DEBUGLOG_DISABLE_LOG
// You can also set default log level by defining macro (default: INFO)
// #define DEBUGLOG_DEFAULT_LOG_LEVEL_WARN // for release version
#define DEBUGLOG_DEFAULT_LOG_LEVEL_TRACE // for debug version
#include <DebugLog.h>                    // https://github.com/hideakitai/DebugLog
```
---
### Troubleshooting
If you get compilation errors, more often than not, you may need to install a newer version of the core for Arduino boards.

Sometimes, the project will only work if you update the board core to the latest version because I am using newly added functions.

---
### Issues
Submit issues to: [ESP32-A2DP-source issues](https://github.com/teamprof/esp32-a2dp-source/issues) 

---
### TO DO
1. Search for bug and improvement.
---

### Contributions and Thanks
Many thanks to the following authors who have developed great audio data and Arduino libraries.
1. [Sound by Pixabay](https://pixabay.com/)
2. [A Simple Arduino Bluetooth Music Receiver and Sender for the ESP32](https://github.com/pschatzmann/ESP32-A2DP)
3. [DebugLog](https://github.com/hideakitai/DebugLog)

Many thanks for everyone for bug reporting, new feature suggesting, testing and contributing to the development of this project.
---

### Contributing
If you want to contribute to this project:

- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library
---

### License
- The project is licensed under GNU GENERAL PUBLIC LICENSE Version 3
---

### Copyright
- Copyright 2023 teamprof.net@gmail.com. All rights reserved.

