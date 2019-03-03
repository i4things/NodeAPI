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


// called when packet received from node
void received(uint8_t buf[], uint8_t size, int16_t rssi) {
  Serial.println("Data Received :");
  for (int i = 0; i < size; i++) {
    Serial.println(buf[i]);
  }
};

///////////////////////////////////////////////////////////////////////////////////
// THING LIB
#include "IoTThing.h"

// make sure you register in www.i4things.com and get your own device ID -
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// also for the wifi API you need to register a gateway for every node and fil gateway ID and gateway KEY
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details

//please create a gateway for every per node and set it here
#define gateway_id 4172
#define gateway_key "6869376AF0D54449C1C22ED5BBA2A18F"

#define ssid  "PLUSNET-7F63NX"
#define pass "4e3acd6a4f"

#define thing_id 37
uint8_t thing_key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
IoTThing thing(ssid, pass, thing_id, thing_key, gateway_id, gateway_key, received);


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




