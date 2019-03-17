Download Arduino IDE:
Download Arduino IDE from https://www.arduino.cc/en/Main/Software

Install ESP8266 board software:
Open Arduino IDE and select File ? Preferences (Arduino ? Preferences on Mac) and add the following text for field Additional Boards Manager URLs: http://arduino.esp8266.com/stable/package_esp8266com_index.json and select OK.
Open Tools ? Boards... ? Boards Manager... and scroll down and click on esp8266 by ESP8266 Community. Click the Install button to download and install the latest ESP8266 board software. Select Close.

Dependacy on ( the following lib need to be isntalled) : https://github.com/i4things/Gateway/blob/master/src/arduino-esp32/DHTesp.zip

Use Board: "WeMos D1 R1" 

Garage example

Scenario : Wemos, DHT22 sensor measuring temperature and humidity and 2 relays

1. Relay 1 is for Automatic Garage door  - when Open/Close button on web page is clicked Relay 1 is ON for 2 sec

2. DTH22 measures temp and humidity and send it every 10 min to the server ( if different)

3. Relay 2 is connected to a fan

4. When humidity over 70% relay 2 is automatically switched ON for 2 hours cycle

5. When clicked on web page - "Relay 2 ON" button Relay 2 is switched ON.

6. When clicked on web page - "Relay 2 OFF" button Relay 2 is switched OFF. ( if humidity is > 70% it will be switched ON automatically after 10 minutes again)

7. Relay 2 cannot stay ON for more then 2 hours every 2.5 hours

8. If Relay 2 is switched OFF because max ON ( 2 hours) is reached it cannot be switched ON for the next 1/2h

* The default pins on D1 Relay Shield and DHT Shiled cannot be used as they clash with serial interface  for logging and built-in led. Please make sure they are not connected and you have actually bridged them as described below  

DOOR Relay controling pin to D7 

FAN Relay controling pin to D6

DHT data pin to D5

Build :

 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/view1.JPG)
 
 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/view2.JPG)
  
 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/view3.JPG)

Pinout:
 
 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/esp8266-wemos-d1-mini-pinout.png)

Parts:

 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/cpu.jpg)

 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/dht.jpg)

 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/triple.jpg)

 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/WEMOS/Garage/thing/relay.jpg)
