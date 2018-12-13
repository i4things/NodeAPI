# Steps to run the examples:

1. Navigate to www.i4things.com

1.1. Sign-in ( free )
1.2. Check your email and activate your registration
1.3. Navigate to USER AREA
1.4. Create a NODE ( choose name for it ) 
1.5. Click on the node in the Nodes List to get to Node Info and make notes of ID and  Key ( Network Key)

**
optional: ( if you do not have a local coverage and you need to have your own gateway)
1.6. Create Gateway in the USER AREA ( set name, coordinates and is it open or private - please choose open to make easy the initial example process)  
/ in case you DO decide to use private gateway then you will need to pass the gateway id when constructing the IoTThing object/
1.7. Click on the gateway in the Gateway List and get to Gateway Info and make notes about gateway ID and Key (Network Key)
1.8. Download GatewayUI.zip and install from it the Gateway Configurator Application ( Windows ) from here : https://github.com/i4things/Gateway
1.9. Run the application and follow the next instructions to configure your new gateway:

- Download and install USB driver for your gateway from: https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers
- Download and install the configurator from from this repository ( GatewayUI)
- Connect the device using USB to micro USB cable.
- Start The configurator
- Click “Refresh”
- Click “Connect”
- Click “Get Configuration”
- IF all is OK the SSID, PASS, GATEWAY ID, GATEWAY KEY, FREQUENCY should be filled with values. - if not check the connection.
- Setup the WiFi configuration: fill SSID and PASS and click “Send to Gateway”.
- Setup Gateway Details: fill GATEWAY ID and GATEWAY KEY ( provided from the user area of www.i4things.com) and click "Send to Gateway”.
- Setup Frequency : fill the FREQUENCY with one of the following : 868.1 , 868.3 or 868.5 ( only this frequencies are supported for private gateway) and click "Send to Gateway”. 10 Finally Click “Get Configuration” and if all is OK the SSID, PASS, GATEWAY ID, GATEWAY KEY, FREQUENCY should have the values you have configured.
- Restart The Gateway (non mandatory)
**

2. Download the example for the breadboard that you have - we have examples for Adafruit32u4 Lora, Heltec Lora 32 and TTGO Lora
/ The library and examples will work with almost all board with LoRa - you just need to pass the appropriate PINS/
3. Compile and upload teh exmaple
4. Open the .html page that in is the example folder in a browser and you will be able to see data from and send data to the node

IMPORTANT: Download and make available the RadioHead library from : https://www.airspayce.com/mikem/arduino/RadioHead/index.html
