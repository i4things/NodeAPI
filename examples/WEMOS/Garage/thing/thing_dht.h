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

// DHT modulle managment
//

#include "DHTesp.h"

DHTesp sensor;

#define DHT_BUF_SIZE 10
uint8_t DHT_BUF_TEMP[DHT_BUF_SIZE];
uint8_t DHT_BUF_HUM[DHT_BUF_SIZE];
uint8_t DHT_BUF_CNT;

//60sec 60 * 1000 (60000)
#define DHT_TIMEOUT  60000
uint32_t DHT_LAST_EXECUTE;

#define DHT_TIMEOUT_CALC  (DHT_TIMEOUT * DHT_BUF_SIZE)
uint32_t DHT_LAST_EXECUTE_CALC;

inline void DHT_init()
{
  DHT_TEMP = 120; // TEMP IN C = DHT_TEMP - 100
  DHT_HUM = 50;

  sensor.setup(DHT_PIN, DHTesp::DHT22);
  delay(2000);
  float h = sensor.getHumidity();
  float t = sensor.getTemperature();
  if (isnan(h) || isnan(t))
  {
    // issue ?
  }
  else
  {
    DHT_TEMP = (uint8_t)(t + 100);
    DHT_HUM = (uint8_t)h;
  }

  DHT_LAST_EXECUTE = millis();
  DHT_LAST_EXECUTE_CALC = millis();

  DHT_BUF_CNT = 0;
}

inline void DHT_insert_sort(uint8_t arr[], uint8_t val, uint8_t count)
{
  int8_t i = count - 1; // needs to be with sign for the logic to work
  for (; (i >= 0)  && (arr[i] > val); i--)
  {
    arr[i + 1] = arr[i];
  }
  arr[i + 1] = val;
}

inline bool DHT_work()
{
  bool ret = false;

  if (((uint32_t)(((uint32_t)millis()) - DHT_LAST_EXECUTE)) >= DHT_TIMEOUT)
  {
    DHT_LAST_EXECUTE = millis();

    float h = sensor.getHumidity();
    float t = sensor.getTemperature();
    if (isnan(h) || isnan(t))
    {
      // issue ?
    }
    else if (DHT_BUF_CNT < DHT_BUF_SIZE)
    {
      DHT_insert_sort(DHT_BUF_TEMP, (uint8_t)(t + 100), DHT_BUF_CNT);
      DHT_insert_sort(DHT_BUF_HUM, (uint8_t)h, DHT_BUF_CNT);
      DHT_BUF_CNT++;
    }
  }

  if (((uint32_t)(((uint32_t)millis()) - DHT_LAST_EXECUTE_CALC)) >= DHT_TIMEOUT_CALC)
  {
    DHT_LAST_EXECUTE_CALC = millis();

    if (DHT_BUF_CNT > 0)
    {
      if ( DHT_TEMP != DHT_BUF_TEMP[DHT_BUF_CNT / 2])
      {
        DHT_TEMP = DHT_BUF_TEMP[DHT_BUF_CNT / 2];
        ret = true;
      }

      if (DHT_HUM != DHT_BUF_HUM[DHT_BUF_CNT / 2])
      {
        DHT_HUM = DHT_BUF_HUM[DHT_BUF_CNT / 2];
        ret = true;
      }
      DHT_BUF_CNT = 0;

      Serial.print("DHT : "); Serial.print((uint16_t)(DHT_TEMP - 100)); Serial.print(" ");Serial.println(DHT_HUM);
    }
  }

  return ret;
}
