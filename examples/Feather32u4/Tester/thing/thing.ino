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
#define LED_PIN LED_BUILTIN

// make sure you register in www.i4things.com and get your own device ID -
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details

#define thing_id 14
uint8_t key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
IoTThing thing(CS_PIN, INT_PIN, RST_PIN, key, thing_id);


///////////////////////////////////////////////////////////////////////////////////
// LED FLASH

#define LED_FLASH_INTERVAL  1000

#define LED_OFF 0
#define LED_ON 1
#define LED_FLASHING 2

uint8_t LED_STATE;
uint8_t LED_LAST;
uint32_t LED_LAST_WORK;


void led_work() {
  if (((uint32_t)(((uint32_t)millis()) - LED_LAST_WORK)) >= LED_FLASH_INTERVAL) {
    LED_LAST_WORK = millis();
    switch (LED_STATE) {
      case 0 : {
          if (LED_LAST != 0) {
            digitalWrite(LED_PIN, LOW);
            LED_LAST = 0;
            Serial.println("OFF");
          }
        }; break;
      case 1 : {
          if (LED_LAST != 1) {
            digitalWrite(LED_PIN, HIGH);
            LED_LAST = 1;
            Serial.println("ON");
          }
        }; break;
      case 2 : {
          if (LED_LAST == 0) {
            digitalWrite(LED_PIN, HIGH);
            LED_LAST = 1;
            Serial.println("ON");
          }
          else {
            digitalWrite(LED_PIN, LOW);
            LED_LAST = 0;
            Serial.println("OFF");
          }
        }; break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////
// CALLBACKS

void ack_callback(int16_t rssi) {
  LED_STATE = LED_ON;
  Serial.println("ACK");
}

void timeout_callback(uint16_t timeout) {
  LED_STATE = LED_FLASHING;
  Serial.println("TOMEOUT");
}

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

  //LED flashing
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  LED_STATE = LED_ON;
  LED_LAST = 1;
  Serial.println("ON");
  LED_LAST_WORK = millis();
  thing.register_timeout(timeout_callback);
  thing.register_ack(ack_callback);
}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP

void loop() {
  led_work();
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

    LED_STATE = LED_OFF;
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




