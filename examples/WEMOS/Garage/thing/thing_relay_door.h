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

// Relay door modulle managment
//

// 1 sec in millis
#define RELAY_DOOR_OPEN_TIMEOUT 2000
uint32_t RELAY_DOOR_OPEN_START;

inline void RELAY_DOOR_init()
{
  RELAY_DOOR_STATE = 0;

  pinMode(RELAY_DOOR_PIN, OUTPUT);
  digitalWrite(RELAY_DOOR_PIN, LOW);
}

inline bool RELAY_DOOR_click()
{
  bool ret = false;
  if (RELAY_DOOR_STATE == 0)
  {
    RELAY_DOOR_STATE = 1;
    RELAY_DOOR_OPEN_START = millis();
    digitalWrite(RELAY_DOOR_PIN, HIGH);

    Serial.println("DOON : ON");
    
    ret = true;
  }
  else
  {
     Serial.println("DOON : ALREADY ON");
  }
  return ret;
}

inline bool RELAY_DOOR_work()
{
  bool ret = false;
  if (RELAY_DOOR_STATE == 1 )
  {
    if (((uint32_t)(((uint32_t)millis()) - RELAY_DOOR_OPEN_START)) >= RELAY_DOOR_OPEN_TIMEOUT)
    {
      RELAY_DOOR_STATE = 0;
      digitalWrite(RELAY_DOOR_PIN, LOW);

      Serial.println("DOON : OFF");
      ret = true;;
    }
  }
  return ret;
}
