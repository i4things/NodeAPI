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
// THING LIB

#include "IoTThing.h"


//Feather32u2 Lora PINS
#define CS_PIN 8
#define RST_PIN 4
#define INT_PIN 7

// make sure you register in www.i4things.com and get your own device ID -
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details

#define thing_id 4
uint8_t key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
IoTThing thing(CS_PIN, INT_PIN, RST_PIN, key, thing_id);


// 2 minutes
#define MESSAGE_INTERVAL  120000
uint32_t MESSAGE_LAST_SEND;

#include "thing_dht.h"

void setup()
{
  Serial.begin(115200);
  
  MESSAGE_LAST_SEND = millis() + MESSAGE_INTERVAL * 2;

  // Initial delay to give chance to the com port to cennect
  delay(2000);
  
  // Radio
  thing.init();

  //DHT (temp/Moist)
  DHT_INIT();

}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP

void loop()
{
  DHT_WORK();

  thing.work();

  // try send message every 2 min
  if (((uint32_t)(((uint32_t)millis()) - MESSAGE_LAST_SEND)) >= MESSAGE_INTERVAL)
  {
    MESSAGE_LAST_SEND = millis();

    uint8_t msg[IoTThing_MAX_MESSAGE_LEN];
    uint8_t msg_size = 0;

    msg[msg_size++] =  (int8_t)DHT_TEMP_GET();
    msg[msg_size++] =  DHT_HUM_GET();

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

  }

  yield();
}




