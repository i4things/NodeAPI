/**
   USE OF THIS SOFTWARE IS GOVERNED BY THE TERMS AND CONDITIONS
   OF THE LICENSE STATEMENT AND LIMITED WARRANTY FURNISHED WITH
   THE PRODUCT.
   <p/>
   IN PARTICULAR, YOU WILL INDEMNIFY AND HOLD B2N LTD., ITS
   RELATED COMPANIES AND ITS SUPPLIERS, HARMLESS FROM AND AGAINST ANY
   CLAIMS OR LIABILITIES ARISING OUT OF THE USE, REPRODUCTION, OR
   DISTRIBUTION OF YOUR PROGRAMS, INCLUDING ANY CLAIMS OR LIABILITIES
   ARISING OUT OF OR RESULTING FROM THE USE, MODIFICATION, OR
   DISTRIBUTION OF PROGRAMS OR FILES CREATED FROM, BASED ON, AND/OR
   DERIVED FROM THIS SOURCE CODE FILE.
*/

///////////////////////////////////////////////////////////////////////////////////
// LOG constants
//#define LOG64_ENABLED
//
//#include <Log64.h>

//Garage example
//
// Scenario : Wemos, DHT22 sensor measuring temperature and humidity and 2 relays
// Relay 1 is for Automatic Garage door  - when Open/Close button on web page is clicked Relay 1 is ON for 2 sec
// DTH22 measures temp and humidity and send it every 10 min to the server ( if different)
// Relay 2 is connected to a fan
// When humidity over 70% relay 2 is automatically switched ON for 2 hours cycle
// When clicked on web page - "Relay 2 ON" button Relay 2 is switched ON.
// When clicked on web page - "Relay 2 OFF" button Relay 2 is switched OFF. ( if humidity is > 70% it will be switched ON automatically after 10 minutes again)
// Relay 2 cannot stay ON for more then 2 hours every 2.5 hours
// if Relay 2 is switched OFF because max ON ( 2 hours) is reached it cannot be switched ON for the next 1/2h
// 
// * The default pins on D1 Relay Shield and DHT Shiled cannot be used as they clash with serial interface 
//   for logging and built-in led
//   Please make sure they are not connected and you have actually bridged them as described below  
//
// DOOR Relay controling pin to D7 
// FAN Relay controling pin to D6
// DHT data pin to D5



//PINS
#define RELAY_DOOR_PIN D7
#define RELAY_FAN_PIN D6
#define DHT_PIN D5

//TRESHOLDS
#define MAX_HUMIDITY 70


///////////////////////////////////////////////////////////////////////////////////
// THING LIB
#include "IoTThing.h"

// make sure you register in www.i4things.com and get your own device ID -
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// also for the wifi API you need to register a gateway for every node and fill gateway ID and gateway KEY
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details


bool FORCE_DATA_MESSAGE;

uint8_t RELAY_DOOR_STATE;
uint8_t RELAY_FAN_STATE;
uint8_t DHT_TEMP; // TEMP IN C = DHT_TEMP - 100
uint8_t DHT_HUM;  // 0 - 100 in %

#include "thing_dht.h"
#include "thing_relay_door.h"
#include "thing_relay_fan.h"

// called when packet received from node
void received(uint8_t buf[], uint8_t size, int16_t rssi)
{
  // is there enough data
  if (size >= 1)
  {
    Serial.print("CALLBACK "); Serial.println((uint16_t)buf[0]);

    if (buf[0] == 0)
    {
      // RELAY DOOR CLICK
      if (RELAY_DOOR_click())
      {
        FORCE_DATA_MESSAGE = true;
        Serial.println("F0RCE MESSAGE from DOOR_click");
      }
    }
    else if (buf[0] == 1)
    {
      if (RELAY_FAN_off())
      {
        FORCE_DATA_MESSAGE = true;
        Serial.println("F0RCE MESSAGE from FAN_off");
      }
    }
    else
    {
      // all else is FAN ON for buf[0] munutes
      if (RELAY_FAN_on(((uint32_t)(buf[0] - 1)) * (1000 * 60)))  // convert minutes to millis
      {
        FORCE_DATA_MESSAGE = true;
        Serial.print("F0RCE MESSAGE from FAN_on for minutes : "); Serial.println(((uint32_t)buf[0]) - 1);
      }
    }
  }
}

#define gateway_id 4172
#define gateway_key "6869376AF0D54449C1C22ED5BBA2A18F"

#define ssid  "PLUSNET-7F63NX"
#define pass "4e3acd6a4f"

#define thing_id 37

uint8_t thing_key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
IoTThing thing(ssid, pass, thing_id, thing_key, gateway_id, gateway_key, received);

// need to be here as thing need to be visible
#include "thing_led.h"

void setup()
{
  // Serial for debug
  Serial.begin(115200);
  delay(2000); // give sometime to serial to connect

  // LED
  LED_init();
  Serial.println("LED Initialized");

  // init request to send data messsage
  FORCE_DATA_MESSAGE = true;
  Serial.println("FORCE MESSAGE Initialized");

  // RELAY DOOR init
  RELAY_DOOR_init();
  Serial.println("DOOR Initialized");

  // RELAY FAN init
  RELAY_FAN_init();
  Serial.println("FAN Initialized");

  // DHT init
  DHT_init();
  Serial.println("DHT Initialized");

  // thing init
  thing.init();
  Serial.println("THING Initialized");

}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP

void loop()
{
  //keep the return bool pattern for consistency ( not used in LED module )
  if (LED_work())
  {
    // should never happened
    Serial.println("F0RCE MESSAGE from LED");
    FORCE_DATA_MESSAGE = true;
  }

  // return true if temp is updated
  if (DHT_work())
  {
    Serial.println("F0RCE MESSAGE from DHT");
    FORCE_DATA_MESSAGE = true;
    if (DHT_HUM > MAX_HUMIDITY)
    {
      RELAY_FAN_on();
    }
  }

  if (RELAY_DOOR_work())
  {
    Serial.println("F0RCE MESSAGE from DOOR");
    FORCE_DATA_MESSAGE = true;
  }

  if (RELAY_FAN_work())
  {
    Serial.println("F0RCE MESSAGE from FAN");
    FORCE_DATA_MESSAGE = true;
  }

  // check if send message is required
  if (FORCE_DATA_MESSAGE)
  {
    FORCE_DATA_MESSAGE = false;

    uint8_t msg[IoTThing_MAX_MESSAGE_LEN];
    uint8_t msg_size = 0;

    msg[msg_size++] = RELAY_DOOR_STATE;
    msg[msg_size++] = RELAY_FAN_STATE;
    msg[msg_size++] = DHT_TEMP;
    msg[msg_size++] = DHT_HUM;

    Serial.print("DOOR :"); Serial.println((RELAY_DOOR_STATE == 1) ? "ON" : "OFF");
    Serial.print("FAN :"); Serial.println((RELAY_FAN_STATE == 1) ? "ON" : "OFF");
    Serial.print("TEMP :"); Serial.println((uint16_t)(DHT_TEMP - 100));
    Serial.print("HUMIDITY :"); Serial.print(DHT_HUM); Serial.println("%");

    // check if IoT layer is ready to accept new message
    if (thing.is_ready())
    {
      thing.send(msg, msg_size);
    }
    else
    {
      // cancel previouse work and send new message
      thing.cancel();
      thing.send(msg, msg_size);
    }

  }

  thing.work();

  yield();
}
