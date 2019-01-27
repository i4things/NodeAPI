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
// DHT ( Humidity/Temp)

#include "DHT.h"

#define DHT_PIN 6
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

//60sec 60 * 1000 (60000)
#define DHT_TIMEOUT  10000
uint32_t DHT_LAST_EXECUTE;

float DHT_TEMP_VALUE;
float DHT_HUM_VALUE;

void DHT_INIT()
{
  //important make sure DHT is initialized

  DHT_TEMP_VALUE = -20;
  DHT_HUM_VALUE = 50;
  dht.begin();
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))
  {
    // log error
  }
  else
  {
    DHT_TEMP_VALUE = t;
    DHT_HUM_VALUE = (uint8_t) h;
  }
}

void DHT_WORK()
{
  if (((uint32_t)(((uint32_t)millis()) - DHT_LAST_EXECUTE)) >= DHT_TIMEOUT)
  {
    DHT_LAST_EXECUTE = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if ((isnan(h)) || (isnan(t)))
    {
     Serial.println("DHT read failed");
    }
    else
    {

      DHT_HUM_VALUE =  h;
      DHT_TEMP_VALUE = t;
   
    }
  }
}

uint8_t DHT_HUM_GET()
{
  return DHT_HUM_VALUE;
}

float DHT_TEMP_GET()
{
  return DHT_TEMP_VALUE;
}





