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
#define LOG64_ENABLED

#include <Log64.h>

///////////////////////////////////////////////////////////////////////////////////
// THING LIB

#include "IoTThing.h"
#include "thing_timestamp.h"
#include "thing_door_lock.h"
#include "thing_door_sensor.h"
#include "thing_light.h"
#include "thing_dht.h"


//Feather32u2 Lora PINS
#define CS_PIN 8
#define RST_PIN 4
#define INT_PIN 7

boolean forceDataMessage = false;

// called when packet received from node
// IMPORTANT!
// Waight for 1 minute to get the data after opening the iot_get_send.html - as data is dispatched every minute from server to gateways
void received(uint8_t buf[], uint8_t size, int16_t rssi)
{

  // is there enough data
  if (size >= 1)
  {
    switch (buf[0])
    {
      case 1 :
        {
          Serial.println("Open Door");
          DOOR_LOCK_OPEN();
          break;
        };
      case 2 :
        {
          Serial.println("Light ON");
          LIGHT_ON();
          forceDataMessage = true;
          break;
        };
      case 3 :
        {
          Serial.println("Light OFF");
          forceDataMessage = true;
          LIGHT_OFF();
          break;
        };

    }

  }
}


// make sure you register in www.i4things.com and get your own device ID -
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details

#define thing_id 18
uint8_t key[16] = {77, 2, 3, 3, 41, 5, 6, 7, 88, 9, 10, 11, 12, 13, 114, 15};
IoTThing thing(CS_PIN, INT_PIN, RST_PIN, key, thing_id, received);


// 2 minutes
#define MESSAGE_INTERVAL  20000
uint32_t MESSAGE_LAST_SEND;



void setup()
{
  Serial.begin(115200);

  MESSAGE_LAST_SEND = millis() + MESSAGE_INTERVAL * 2;

  // Initial delay to give chance to the com port to cennect
  delay(2000);

  // Radio
  thing.init();
 
  //Timestamp
  TIMESTAMP_INIT();
  
  //DHT (temp/Moist)
  DHT_INIT();

  //Door Lock
  DOOR_LOCK_INIT();

  //Light
  LIGHT_INIT();

   //Door Sensor
  DOOR_SENSOR_INIT();

}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP

void loop()
{
  TIMESTAMP_WORK();
  
  DHT_WORK();

  DOOR_LOCK_WORK();

  DOOR_SENSOR_WORK();

  LIGHT_WORK();

  thing.work();

  // try send message every 2 min
  if ((((uint32_t)(((uint32_t)millis()) - MESSAGE_LAST_SEND)) >= MESSAGE_INTERVAL ) || forceDataMessage)
  {
    MESSAGE_LAST_SEND = millis();

    uint8_t msg[IoTThing_MAX_MESSAGE_LEN];
    uint8_t msg_size = 0;

    msg[msg_size++] = (int8_t)DHT_TEMP_GET();
    msg[msg_size++] = DHT_HUM_GET();
    msg[msg_size++] = IS_LIGHT_ON() ? 1 : 0;
    msg[msg_size++] = IS_DOOR_OPEN() ? 1 : 0;
    

    thing.add_uint(msg, msg_size, GET_DOOR_SENSOR_LAST_OPEN());

    thing.add_uint(msg, msg_size, GET_DOOR_SENSOR_LAST_CLOSE());


    for (int i = 0; i < msg_size; i++)
    {
      Serial.println(msg[i]);
    }

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
    
    forceDataMessage = false;

  }

  yield();
}




