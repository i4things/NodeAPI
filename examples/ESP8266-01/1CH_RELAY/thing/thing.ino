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

// FOR LCH RELAY MODULE
//#define LCH

// FOR SONOFF
#define SONOFF

uint8_t RELAY1_STATE; // 0-OFF  1-ON

#if defined(LCH)
//ESP8266-01 1 CHANNEL RELAYY
// relay,0,1 = A0 01 01 A2
// relay,0,0 = A0 01 00 A1
#define RELEAY_COMMAND_SIZE 4

uint8_t RELAY1_ON[RELEAY_COMMAND_SIZE] =  { 0xA0, 0x01, 0x01, 0xA2};
uint8_t RELAY1_OFF[RELEAY_COMMAND_SIZE] = { 0xA0, 0x01, 0x00, 0xA1};
#endif

#if defined(SONOFF)
#define RELAY_PIN 12
#define LED_PIN 13
#define BUTTON_PIN 0
uint8_t BUTTON_DOWN; // 0 - UP , 1 - DOWN
#define BUTTON_TIMEOUT  100
uint32_t BUTTON_LAST_EXECUTE;
#endif

///////////////////////////////////////////////////////////////////////////////////
// THING LIB
#include "IoTThing.h"

// make sure you register in www.i4things.com and get your own device ID -
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// also for the wifi API you need to register a gateway for every node and fill gateway ID and gateway KEY
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details


bool FORCE_DATA_MESSAGE;

// called when packet received from node
void received(uint8_t buf[], uint8_t size, int16_t rssi)
{
  // is there enough data
  if (size >= 1)
  {
    switch (buf[0])
    {
      case 0 :
        {
          // OFF
          RELAY1_STATE = 0;
#if defined(LCH)
          for (uint8_t i = 0; i < RELEAY_COMMAND_SIZE; i++)
          {
            Serial.write(RELAY1_OFF[i]);
          }
#endif
#if defined(SONOFF)
          digitalWrite(RELAY_PIN, LOW);
          digitalWrite(LED_PIN, HIGH);
#endif
          FORCE_DATA_MESSAGE = true;
          break;
        }
      case 1 :
        {
          // ON
          RELAY1_STATE = 1;
#if defined(LCH)
          for (uint8_t i = 0; i < RELEAY_COMMAND_SIZE; i++)
          {
            Serial.write(RELAY1_ON[i]);
          }
#endif
#if defined(SONOFF)
          digitalWrite(RELAY_PIN, HIGH);
          digitalWrite(LED_PIN, LOW);
#endif
          FORCE_DATA_MESSAGE = true;
          break;
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


void setup()
{
#if defined(LCH)
  // init serial
  Serial.begin(9600);
  // Initial delay to give chance to the com port to connect
  delay(2000);
#endif

#if defined(SONOFF)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(BUTTON_PIN, INPUT);
  BUTTON_LAST_EXECUTE = millis();
  BUTTON_DOWN = 0;
#endif
  // init request to send data messsage
  FORCE_DATA_MESSAGE = true;
  // initt state
  RELAY1_STATE = 0;
  // thing init
  thing.init();
}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP

void loop()
{
#if defined(SONOFF)
  //deal with the button ( as a manual switch)
  if (((uint32_t)(((uint32_t)millis()) - BUTTON_LAST_EXECUTE)) >= BUTTON_TIMEOUT)
  {
    BUTTON_LAST_EXECUTE = millis();
    if (digitalRead(BUTTON_PIN) == LOW)
    {
      BUTTON_DOWN = 1;
    }
    else
    {
      if (BUTTON_DOWN == 1)
      {
        // button released toggle relay
        if (RELAY1_STATE == 1)
        {
          RELAY1_STATE = 0;
          digitalWrite(RELAY_PIN, LOW);
          digitalWrite(LED_PIN, HIGH);
        }
        else
        {
          RELAY1_STATE = 1;
          digitalWrite(RELAY_PIN, HIGH);
          digitalWrite(LED_PIN, LOW);
        }
        FORCE_DATA_MESSAGE = true;
      }
      BUTTON_DOWN = 0;
    }
  }
#endif

  // check if send message is required
  if (FORCE_DATA_MESSAGE)
  {
    FORCE_DATA_MESSAGE = false;

    uint8_t msg[IoTThing_MAX_MESSAGE_LEN];
    uint8_t msg_size = 0;

    msg[msg_size++] = RELAY1_STATE;

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
