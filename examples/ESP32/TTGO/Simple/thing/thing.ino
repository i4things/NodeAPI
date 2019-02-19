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


//TTGO Lora PINS
#define CS_PIN 18
#define RST_PIN 14
#define INT_PIN 26

// make sure you register in www.i4things.com and get your own device ID - 
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details

#define thing_id 1
uint8_t key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
IoTThing thing(CS_PIN, INT_PIN, RST_PIN, key, thing_id);


// 2 minutes
#define MESSAGE_INTERVAL  120000
uint32_t MESSAGE_LAST_SEND;

void setup() {
  // make sure we send a messae imediatly after start
  MESSAGE_LAST_SEND = millis() + MESSAGE_INTERVAL * 2;

  // init serial
  Serial.begin(115200);
   // Initial delay to give chance to the com port to cennect
  delay(2000);
  // thing init
  thing.init();

}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP

void loop() {
  thing.work();

  // try send message every 2 min
  if (((uint32_t)(((uint32_t)millis()) - MESSAGE_LAST_SEND)) >= MESSAGE_INTERVAL) {
    MESSAGE_LAST_SEND = millis();

    uint8_t msg[IoTThing_MAX_MESSAGE_LEN];

    msg[0] = random(30); // e.g. temp
    msg[1] =  random(50, 90); // e.g. humidity

    // log what we send
    Serial.println("Sending data : ");
    for (int i = 0; i < 2; i++) {
      Serial.println(msg[i]);
    }

    // check if IoT layer is ready to accept new message
    if (thing.is_ready()) {
      thing.send(msg, 2);
    }
    else {
      // cancel previouse work and send new message
      thing.cancel();
      thing.send(msg, 2);
    }
  }

  yield();
}




