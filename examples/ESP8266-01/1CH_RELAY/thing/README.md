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
 CPU Frequency: "80 MHz' 
 Crystal Frequency: @26 MHz' 
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
 
 Sonoff Basivc:
<IMAGE: Sonoff_ESP8266.jpg>
Note: newer version of the Sonoff module consist of five pins below the button. Follow the image above and ignore the pin furthest to the Button.

Do not connect AC power during the flash cable connection.


Sonoff Dual:
<IMAGE: Sonoff_Dual_ESP8266.jpgg>
Dual GPIO0 grounded Programming the Sonoff Dual is also more difficult as the button is not connected to GPIO0 which is needed to put the ESP8266 in programming mode during power up.
I suggest to solder a 4 pin header for the serial interface as shown in the image on the right (the vertical connector) and use the small inter layer VIA to ground GPIO0 using the GND pin from the button 0 and button 1 header.

The 4 pin header in the middle, which is normally not present, is not needed but might be used in programming the ESP8266 as there must be a better way to get the initial code loaded ...
