# SentinelSuite
## IoT Project for Server Room Surveillance

This project is created and build with PlatformIo 

In folder esp32 and folder esp8266 create a file called **secrets.h** and insert this code: 
```c
#pragma once
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
```

On ESP32 flash code in ESP32 folder. 
It implements environmental policy using this sensor and actuators: 
* DHT11 for temperature and humidity
* Water sensor for measuring water level
* Flame sensor for detect flames 
* RGB and SMD led for simulating different actuators

On ESP8266 flash code in ESP8266 folder. It implements access control policy using this sensors and actuators: 
* RFID reader 
* RGB led
* Buzzer


### Raspberry Pi
Install:
* mosquitto
```bash
sudo apt install mosquitto mosquitto-clients
```
It is used as broker MQTT 
* Node-RED using npm
```bash
sudo npm install -g --unsafe-perm node-red
```
Is used for create the dashboard and logic of the program 

In a terminal run:
```bash
python3 stream-video.py
```

In other terminal run: 
```bash
node-red
```
Once open in localhost:1880 import **flow.json** and open the Dashboard view




