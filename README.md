# Smart-Sensor-for-Smart-Home-IoT-Applications

Hardware:
  - 1x ESP32 Devkit V1
  - 2x DHT22 temperature & humidity sensor
  - 1x BH1750 light sensor
  - 1x MQ135 gas sensor
  
OS: Real Time Operaing System (RTOS) built-in ESP32.

Data statistics & analytics on ThinkSpeak platform.

Functions:
  - Measure temperature & humidity; light intensity; gas concentration.
  - Estimate Error for DHT measurement results.
  - Auto Compute & Adjust HeatIndex.
  - Monitor data from anywhere on ThinkSpeak.
  
Note: this repo is program for ESP32 devkit V1, doesn't include schematic and circuit diagram but you can easy find how to connect these sensor to ESP on Google. Additionally, you can use any ESP32 or ESP8066 instead, but please make sure you connect and define, initialize GPIO pins correctly.
