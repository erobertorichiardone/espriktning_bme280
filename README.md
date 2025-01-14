# espriktning bme280
ESP8266 based board replacement for Vindriktning with PM1006 and BME280 sensors

All started from Hypfer to send PM2.5 values to MQTT server

[https://github.com/Hypfer/esp8266-vindriktning-particle-sensor](https://github.com/Hypfer/esp8266-vindriktning-particle-sensor)


Then some custom PCB to fit inside the case, because adding some object in the case seems to modify PM2.5 readings.

[https://hackaday.io/project/181195-ikea-vindriktning-pcb](https://hackaday.io/project/181195-ikea-vindriktning-pcb)


But we need to read the PM2.5 values from the front of the device, not only a generic light!
So why not completely replacing IKEA PCB? there is the notmart ESPriktning project that still uses an ESP8266 board on a single completely new board

[https://github.com/notmart/espriktning](https://github.com/notmart/espriktning)

<img src="https://github.com/erobertorichiardone/espriktning_bme280/blob/master/images/espriktning.jpeg" width="300">


At this point, adding an additional sensor to measure temperature, humidity and pressure is simple using a small BME280 sensor. This fork adds MQTT data pushing if the BME280 is connected, and a display notification also for temperature.


Board inside:

<img src="https://github.com/erobertorichiardone/espriktning_bme280/blob/master/images/espriktning_bme280_0.jpeg" width="300">


PM2.5 local display:

<img src="https://github.com/erobertorichiardone/espriktning_bme280/blob/master/images/espriktning_bme280_2.jpeg" width="300">


Temperature local display:

<img src="https://github.com/erobertorichiardone/espriktning_bme280/blob/master/images/espriktning_bme280_3.jpeg" width="300">




## Board deployment

Use Arduino 1.8.0 onward

Add ESP8266 board (File -> Preferences -> Additional Boards Manager URLs)
* http://arduino.esp8266.com/stable/package_esp8266com_index.json

Add following libaries (Tools -> Library):
* 	WiFiMQTTManager Library 1.0.1-beta
* 	WiFiManager 2.0.11-beta
* 	PubSubClient 2.8.0
* 	BME280 3.0.0 by Tyler Glenn
* 	NeoPixelBus by Makuna 2.7.0

Add the board (Tools -> Board -> Boards Manager)
* 	esp8266 3.0.2
	
Choose a compatible board (Tools -> Board -> ESP8266 Boards (3.0.2) )
* 	Generic ESP8266 Module or LOLIN(WEMOS) D1 mini (clone) 


## User configuration

At the first start, the board enables an WiFi Access Point.
Join the WiFi network, that has name "ESPriktning" with no password.
Access with web browser the address:
>  http://192.168.4.1/ 

Here under the "Configure WiFi" page you can configure:
* SSID and WPA password of your WiFi network
* IP address and TCP port of your MQTT server
* A configurable topic prefix
* Optionally username and password for MQTT authentication
 
### Measurement to MQTT

The MQTT values will be pushed to the server on the following topics:
* PM2.5 in ug/m^3 ->   PREFIX/pm2_5
* temperature in °C ->   PREFIX/temp
* pressure in hPa ->   PREFIX/pres
* humidity in % ->   PREFIX/hum

Sample publish values:

```
err@elitedesk:~$ mosquitto_sub -v -h localhost -t '#'
esprikningstudio/pm2_5 11
esprikningstudio/temp 32.92
esprikningstudio/pres 980.68
esprikningstudio/hum 25.13
```

If BME280 is not found, temperature, pressure and humidity are not sent.


### Reset of settings

Can be accomplished:
1. From Arduino, under Tools -> Erase Flash -> All Flash Contents
2. Physically pressing the SW2 flash pins for more that 2 seconds
3. From Arduino serial connection

## Serial interface

Write "help" from serial monitor interface. Speed is 115200 baud.

```
help
Commands:
help                   Shows this help
reboot                 Reboots the device
printwifisettings      Prints SSID and password of the current wifi connection
connectwifi SSID pass  Attempts to connect to a new wifi network
testnumber  num        Tests the number and color animation with the given 0-99 number
printsettings          Prints all available settings
get config_key         Prints the value of the given config key
set config_key value   Sets the value of the given config key to the given value
factoryreset           Forgets wifi and other settings, starts as new
turning fan off
```

## Greetings

This project exists thankfully to:
<a href="http://avvoca.to.it/">DLx</a>
<a href="http://notmart.org/">notmart</a>





