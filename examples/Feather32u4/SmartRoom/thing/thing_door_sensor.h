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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Door Lock


#define DOOR_SENSOR_PIN 5

int DOOR_SENSOR_STATE;

uint32_t DOOR_SENSOR_LAST_OPEN;
uint32_t DOOR_SENSOR_LAST_CLOSE;

void DOOR_SENSOR_INIT()
{
  pinMode(DOOR_SENSOR_PIN, INPUT);
  DOOR_SENSOR_LAST_OPEN = GET_TIMESTAMP_NOW();
  DOOR_SENSOR_LAST_CLOSE = GET_TIMESTAMP_NOW();
}

void DOOR_SENSOR_WORK()
{
  int OLD_STATE =  DOOR_SENSOR_STATE;
  DOOR_SENSOR_STATE = digitalRead(DOOR_SENSOR_PIN);
  if(OLD_STATE == HIGH && DOOR_SENSOR_STATE == LOW)
  {
      //the door just got opened
      DOOR_SENSOR_LAST_OPEN = GET_TIMESTAMP_NOW();
      Serial.println("Door Open Detected");
  }
  else if (OLD_STATE == LOW && DOOR_SENSOR_STATE == HIGH)
  {
     //the door just got closed
     DOOR_SENSOR_LAST_CLOSE = GET_TIMESTAMP_NOW();
     Serial.println("Door Close Detected");
  }
}

bool IS_DOOR_OPEN()
{
  return DOOR_SENSOR_STATE==LOW?true:false;
}

uint32_t GET_DOOR_SENSOR_LAST_OPEN()
{
  uint32_t last_open = ((uint32_t)(GET_TIMESTAMP_NOW()/1000)) - ((uint32_t)(DOOR_SENSOR_LAST_OPEN/1000)); //in sec   
  return last_open;
}

uint32_t GET_DOOR_SENSOR_LAST_CLOSE()
{
  uint32_t last_close = ((uint32_t)(GET_TIMESTAMP_NOW()/1000)) - ((uint32_t)(DOOR_SENSOR_LAST_CLOSE/1000));  //in sec
  return last_close;
}



