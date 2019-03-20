Download Arduino IDE:
Download Arduino IDE from https://www.arduino.cc/en/Main/Software

Install ESP8266 board software:
Open Arduino IDE and select File ? Preferences (Arduino ? Preferences on Mac) and add the following text for field Additional Boards Manager URLs: http://arduino.esp8266.com/stable/package_esp8266com_index.json and select OK.
Open Tools ? Boards... ? Boards Manager... and scroll down and click on esp8266 by ESP8266 Community. Click the Install button to download and install the latest ESP8266 board software. Select Close.

Upload Sketch: 
Easiest way is to use "CH340 USB to ESP8266 Serial ESP-01" adapter with on-board toggle switch between UART side for serial TTL debugging and PROG for firmware programming / you can also can use any USB to TTL adapter - plenty of instructions online how to upload from Arduino IDE to ESP8266-01 board/

USB Driver :
Most of the USB adapters will rrequire to installl theh following driver :  https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers

Arduinoo IDE Bor Settings:

 Board: "Generic ESP8266 Module" 
 
 Upload Speed: "115200"
 
 Upload Using: "Serial" 
 
 CPU Frequency: "80 MHz" 
 
 Crystal Frequency: @26 MHz"
 
 Flash Size: "IM (no SPIFFS)" 
 
 Flash Mode: "DOUT" 
 
 Flash Frequency: "40MHz" 
 
 Reset Method: "ck" 
 
 Debug port: "Disabled" 
 
 Debug Level: "None" 
 
 IwIP Variant: "v2 Lower Memoyy" 
 
 VTables: "Flash" 
 
 Exceptions: "Disabled" 
 
 Builtin Led: "2" 
 
 Erase Flash: "Only Sketch" 
 
 Port: "COM9" 
 
 Get Board Info 
 
 
 Switch the code between LCH Relay Modulle and Sonooff

for LCH uncomment #define LCH and comment out the SONOFF 
for SONOFF uncomment #define SONOFF and comment out the LCH  

 For All Sunoff Modules :
  
  To upload :

1. Do not plug into the USB on your computer.

2. Do not connect the Sonoff to mains power.

3. Press and hold the push button on the Sonoff board.

4. Insert the FTDI converter USB in your computer (while holding the push button).

5. After 2–3 seconds, release the push button.


Do not connect AC power during the flash cable connection.
 
 Sonoff Basic:
 
 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266.jpg)
 
 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_PCB.jpg)
 
 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_CH340G_1.jpg)
 
  ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_CH340G_2.jpg)
 

Note: newer version of the Sonoff module consist of five pins below the button. Follow the image above and ignore the pin furthest to the Button.



Sonoff Dual:

 ![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_Dual_ESP8266.jpg)
 
Dual GPIO0 grounded Programming the Sonoff Dual is also more difficult as the button is not connected to GPIO0 which is needed to put the ESP8266 in programming mode during power up.
I suggest to solder a 4 pin header for the serial interface as shown in the image on the right (the vertical connector) and use the small inter layer VIA to ground GPIO0 using the GND pin from the button 0 and button 1 header.

The 4 pin header in the middle, which is normally not present, is not needed but might be used in programming the ESP8266 as there must be a better way to get the initial code loaded ...

S20 Smart Socket

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_Socket_ESP8266.jpg)

The picture on the right, shows how to program the S20 Smart Socket powered by the FTDI USB converter.
Remember that during programming the Smart Socket is NOT connected to mains.

S26 Smart Socket

The s26 is quite challenging - following is a  step by step picture based instructions

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_1.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_2.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_3.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_4.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_5.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_6.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_7.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_8.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_9.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_10.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_11.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_12.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/Sonoff_ESP8266_S26_13.jpg)

Smart Lighting Base (TYWE2S)

Make sure that you choose Arduino IDE board profile : Generic ESP8295 Module / 1MB (no SPIFFS)

Is also a challenge - the two plastic parts are actually glued together and you need to break them apart using a  screwdriver or knife.

Comment out Sonoff and LCH and uncomment TYWE2S.

// FOR LCH RELAY MODULE
//#define LCH

// FOR SONOFF
//#define SONOFF

// FOR TYWE2S
#define TYWE2S
 
After that follow the picture instructions below.  

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/TYWE2S_1.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/TYWE2S_2.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/TYWE2S_3.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/TYWE2S_4.jpg)

![Cat](https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/TYWE2S_5.jpg)



