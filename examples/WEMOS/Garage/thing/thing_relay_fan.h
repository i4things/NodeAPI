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

// Relay fan modulle managment
//

// last 2.5h  = 150min
#define RELAY_FAN_LAST_SIZE 150
// how many minutes is required to be off from the 150 min
#define RELAY_FAN_REQUIRED_OFF 30
uint8_t RELAY_FAN_LAST[RELAY_FAN_LAST_SIZE];

// 1 minute in millis
#define RELAY_FAN_1_MIN (1000 * 60)
uint32_t RELAY_FAN_LAST_CALC;

uint32_t RELAY_FAN_TIMEOUT;
uint32_t RELAY_FAN_ON_START;

//RELAY_FAN_REQUIRED_OFF minuutes in millis
#define RELAY_FAN_FORCED_OFF_TIMEOUT (1000 * 60 * RELAY_FAN_REQUIRED_OFF)

// o - not forced - 1 forced off
uint8_t RELAY_FAN_FORCED_OFF;
uint32_t RELAY_FAN_FORCED_OFF_START;


// one minute in millis
#define RELAY_FAN_MIN_ON_TIME (1000 * 60)

// 2h in millis
#define RELAY_FAN_MAX_ON_TIME (1000 * 60 * 120)

inline void RELAY_FAN_init()
{
  RELAY_FAN_STATE = 0;
  RELAY_FAN_FORCED_OFF = 0;

  pinMode(RELAY_FAN_PIN, OUTPUT);
  digitalWrite(RELAY_FAN_PIN, LOW);

  memset(RELAY_FAN_LAST, 0, RELAY_FAN_LAST_SIZE);
  RELAY_FAN_LAST_CALC = millis();
}

// true if the requreed time been off is achieved - e.g. 30 min off every 2 hours
inline bool RELAY_FAN_ON_check()
{
  bool ret = false;
  uint16_t on_minutes = 0;
  for (uint16_t i = 0; i <  RELAY_FAN_LAST_SIZE; i++ )
  {
    on_minutes += RELAY_FAN_LAST[i];
  }
  if ((RELAY_FAN_LAST_SIZE - on_minutes)  >= RELAY_FAN_REQUIRED_OFF)
  {
    ret = true;
  }
  return ret;
}

inline bool RELAY_FAN_off()
{
  bool ret = false;
  if (RELAY_FAN_STATE == 1)
  {
    RELAY_FAN_STATE = 0;
    digitalWrite(RELAY_FAN_PIN, LOW);

    Serial.println("FAN : OFF");

    ret = true;
  }
  else
  {
    Serial.println("FAN : ALREADY OFF");
  }
  return ret;
}

// switch releay ON for timeout in millis
inline bool RELAY_FAN_on(uint32_t timeout = RELAY_FAN_MAX_ON_TIME)
{
  bool ret = false;
  if (RELAY_FAN_STATE == 0)
  { // we are not allowed to get on if in forced off state
    if (RELAY_FAN_FORCED_OFF == 0)
    {
      if (RELAY_FAN_ON_check())
      {
        // make sure the timeout is between min and max on time
        if (timeout < RELAY_FAN_MIN_ON_TIME)
        {
          timeout = RELAY_FAN_MIN_ON_TIME;
        }
        else if (timeout > RELAY_FAN_MAX_ON_TIME)
        {
          timeout = RELAY_FAN_MAX_ON_TIME;
        }

        RELAY_FAN_STATE = 1;

        RELAY_FAN_TIMEOUT = timeout;
        RELAY_FAN_ON_START = millis();

        digitalWrite(RELAY_FAN_PIN, HIGH);

        Serial.print("FAN : ON ["); Serial.print(timeout); Serial.println("]");

        ret = true;
      }
      else
      {
        Serial.println("FAN : CANNOT BE SWITCHED ON - TOO MUCH RINUNG");
      }
    }
    else
    {
      Serial.println("FAN : CANNOT BE SWITCHED ON - IN FORCE OFF");
    }
  }
  else
  {
    Serial.println("FAN : ALREADY OFF");
  }
  return ret;
}

inline bool RELAY_FAN_work()
{
  bool ret = false;

  if (((uint32_t)(((uint32_t)millis()) - RELAY_FAN_LAST_CALC)) >= RELAY_FAN_1_MIN)
  {
    RELAY_FAN_LAST_CALC = millis();
    // update on/off history
    memmove(RELAY_FAN_LAST, &RELAY_FAN_LAST[1], RELAY_FAN_LAST_SIZE - 1);
    RELAY_FAN_LAST[RELAY_FAN_LAST_SIZE - 1] = RELAY_FAN_STATE;
    Serial.println("FAN : ADD LAST MINUTE TO HISTORY");
    
    // chek if we have run for too much for last period and need to switch of for a while
    if (RELAY_FAN_STATE == 1)
    {
      if (!RELAY_FAN_ON_check())
      {
        // yes we need to switch off - make sure we switch off for at least the required period
        RELAY_FAN_off();
        RELAY_FAN_FORCED_OFF = 1;
        RELAY_FAN_FORCED_OFF_START = millis();
        
        Serial.println("FAN : TOO MUCH RINUNG - SWITCHED OFF");
        
        ret = true;
      }
    }
  }

  if (RELAY_FAN_STATE == 1)
  {
    // check when to switch it off
    if (((uint32_t)(((uint32_t)millis()) - RELAY_FAN_ON_START)) >= RELAY_FAN_TIMEOUT)
    {
      // yep gone switch off
      RELAY_FAN_off();

      Serial.println("FAN : REQUIRED RINUNG PERIOD PASSED - SWITCHED OFF");
      
      ret = true;
    }
  }

  if (RELAY_FAN_FORCED_OFF == 1)
  {
    // check is forced off time passed
    if (((uint32_t)(((uint32_t)millis()) - RELAY_FAN_FORCED_OFF_START)) >= RELAY_FAN_FORCED_OFF_TIMEOUT)
    {
      // yep - get out of forced off and enable on again
      RELAY_FAN_FORCED_OFF = 0;

      Serial.println("FAN : REQUIRED FORCED OFF PERIOD PASSED - SWITCH ON ALLOWED");
    }
  }

  return ret;
}
