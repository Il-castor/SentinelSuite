# SentinelSuite
## IoT Project for Server Room Surveillance

This project is created and build with PlatformIo 

On ESP32 flash code in ESP32 folder. It implements environmental policy using this sensor and actuators: 
* DHT11 for temperature and humidity
* Water sensor for measuring water level
* Flame sensor for detect flames 
* RGB and SMD led for simulating different actuators

On ESP8266 flash code in ESP8266 folder. It implements access control policy using this sensors and actuators: 
* RFID reader 
* RGB led
* Buzzer

On Raspberry install:
* mosquitto using apt 
* Node-RED using npm 

Mosquitto is used as a broker MQTT and Node-RED is used for create the dashboard and logic of the program 

On Raspberry run: node-red, import flow.json and stream-video.py

