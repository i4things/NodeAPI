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


//ESP32 ULP ( Ultra Low Power) Example
//
//Stay 5 min in ULP mode and blink the built-in led during that time - once every 5 sec
//Wakeup every 5 min and send random temp and humidity to the server using i4things API and again go in ULP mode 


///////////////////////////////////////////////////////////////////////////////////
// ULP section

// https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf

// RTC_MUX Pin Summary
//                                       |  Analog Function
//RTC GPIO Num | GPIO Num  | Pad Name    |  1          |   2      | 3
//0            | 36        | SENSOR_VP   | ADC_H       | ADC1_CH0 | -
//1            | 37        | SENSOR_CAPP | ADC_H       | ADC1_CH1 | -
//2            | 38        | SENSOR_CAPN | ADC_H       | ADC1_CH2 | -
//3            | 39        | SENSOR_VN   | ADC_H       | ADC1_CH3 | -
//4            | 34        | VDET_1      | -           | ADC1_CH6 | -
//5            | 35        | VDET_2      | -           | ADC1_CH7 | -
//6            | 25        | GPIO25      | DAC_1       | ADC2_CH8 | -
//7            | 26        | GPIO26      | DAC_2       | ADC2_CH9 | -
//8            | 33        | 32K_XN      | XTAL_32K_N  | ADC1_CH5 | TOUCH8
//9            | 32        | 32K_XP      | XTAL_32K_P  | ADC1_CH4 | TOUCH9
//10           | 4         | GPIO4       | -           | ADC2_CH0 | TOUCH0
//11           | 0         | GPIO0       | -           | ADC2_CH1 | TOUCH1
//12           | 2         | GPIO2       | -           | ADC2_CH2 | TOUCH2
//13           | 15        | MTDO        | -           | ADC2_CH3 | TOUCH3
//14           | 13        | MTCK        | -           | ADC2_CH4 | TOUCH4
//15           | 12        | MTDI        | -           | ADC2_CH5 | TOUCH5
//16           | 14        | MTMS        | -           | ADC2_CH6 | TOUCH6
//17           | 27        | GPIO27      | -           | ADC2_CH7 | TOUCH7


#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <soc/rtc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/rtc_io_reg.h>
#include <soc/soc_ulp.h>
#include <esp32/ulp.h>
#include <Arduino.h>


// Definitions built-in led on Heltec (pin 25) accfording to the table - RTC_MUX Pin Summary
#define LED_RTC_GPIO_Num 6
#define LED_GPIO_NUM_PIN GPIO_NUM_25

void blink_while_wait_ulp(uint32_t timeout_on,
                          uint32_t timeout_off,
                          uint32_t loop_count)
{

  const uint16_t ULP_DATA_OFFSET = 0;
  const uint16_t ULP_CODE_OFFSET = 3;

  const ulp_insn_t program[] =
  {
    I_MOVI(R1, ULP_DATA_OFFSET), // R1 <= address of data

    I_LD(R2, R1, 2),   // R2 <= loop - how many time to loop  ( third parameter - index 2)

    M_LABEL(1),        // label_loop:

    // SWITCH LED ON AND WEIGHT timeout_on msec
    I_WR_REG_BIT(RTC_GPIO_OUT_W1TS_REG, RTC_GPIO_OUT_DATA_W1TS_S + LED_RTC_GPIO_Num, 1), // LED ON ( SET TO HIGH )

    I_LD(R3, R1, 0),   // R3 <= timeout_on - time to stay ON in msec ( first parameter - index 0)

    M_LABEL(3),        // label_on_wait_loop:
    I_DELAY(25000),    // wait 1 msec
    I_SUBI(R3, R3, 1), // R3 = R3 - 1
    M_BXZ(4),          // IF R3 == 0 THEN GOTO label_exit_on_wait_loop
    M_BX(3),           // GOTO label_on_wait_loop
    M_LABEL(4),        // label_exit_on_wait_loop:

    // SWITCH LED OFF AND WEIGHT timeout_off msec
    I_WR_REG_BIT(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + LED_RTC_GPIO_Num, 1), // LED OFF ( SET TO LOW );

    I_LD(R3, R1, 1),   // R3 <= timeout_off - time to stay OFF in msec ( second parameter - index 1)

    M_LABEL(5),        // label_off_wait_loop:
    I_DELAY(25000),    // wait 1 msec
    I_SUBI(R3, R3, 1), // R3 = R3 - 1
    M_BXZ(6),          // IF R3 == 0 THEN GOTO label_exit_off_wait_loop
    M_BX(5),           // GOTO label_off_wait_loop
    M_LABEL(6),        // label_exit_off_wait_loop:

    // CHECK IF WE HAVE CYCLED ENOUGH
    I_SUBI(R2, R2, 1), // R2 = R2 - 1
    M_BXZ(2),          // IF R2 == 0 THEN GOTO label_exit_loop:
    M_BX(1),           // GOTO label_loop
    M_LABEL(2),        // label_exit_loop:

    // MAKE ONE LAST 5000MSEC DELAY TO COOL THE LED
    I_MOVI(R3, 500),  // R3 <= 500
    M_LABEL(7),        // label_delay_loop:
    I_DELAY(25000),    // wait 1 msec
    I_SUBI(R3, R3, 1), // R3 = R3 -1 
    M_BXZ(8),          // IF R3 == 0 THEN GOTO label_exit_delay_loop:
    M_BX(7),           // GOTO label_delay_loop
    M_LABEL(8),        // label_exit_delay_loop:

    // WAIT UNTIL MAIN CPU CAN BE WAKEUP
    M_LABEL(9),        // label_ready_wakeup_loop:
    I_RD_REG(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP_S, RTC_CNTL_RDY_FOR_WAKEUP_S), // read if ready to wakeup 
    I_ANDI(R0, R0, 1), // check if ready
    M_BXZ(9),          // if not ready GOTO label_ready_wakeup_loop

    // MAIN CPU(CORES) WAKEUP PROCEDURE
    I_WAKE(),
    I_END(),
    I_HALT()
  };

  size_t size = sizeof(program) / sizeof(ulp_insn_t);

  RTC_SLOW_MEM[ULP_DATA_OFFSET] = (RTC_FAST_CLK_FREQ_APPROX / 25000) * timeout_on / 1000;
  RTC_SLOW_MEM[ULP_DATA_OFFSET + 1] = (RTC_FAST_CLK_FREQ_APPROX / 25000) * timeout_off / 1000;
  RTC_SLOW_MEM[ULP_DATA_OFFSET + 2] = loop_count;
  if (ulp_process_macros_and_load(ULP_CODE_OFFSET, program, &size) != ESP_OK) {
    Serial.println(F("Error loading ULP code!"));
    return;
  }
  esp_sleep_enable_ulp_wakeup();
  if (ulp_run(ULP_CODE_OFFSET) != ESP_OK) {
    Serial.println(F("Error running ULP code!"));
    return;
  }
  
  Serial.println(F("Executing ULP code"));
  Serial.flush();
  
  esp_deep_sleep_start();
}


///////////////////////////////////////////////////////////////////////////////////
// THING LIB
#include "IoTThing.h"

// make sure you register in www.i4things.com and get your own device ID -
// the key(private key) - is random bytes choose by you - they need to be the same here and in the iot_get_send.html file)
// also for the wifi API you need to register a gateway for every node and fill gateway ID and gateway KEY
// you also need to set the network key in the iot_get_send.html - which can be obtained when creating new node in the www.i4things.com client area
// in node details


bool go_to_ulp;

void ack_callback(int16_t rssi_)
{
  Serial.println("ACK Received");
  go_to_ulp = true;
}

void timeout_callback(uint16_t timeout_)
{
  Serial.println("MSG Timeout");
  go_to_ulp = true;
}

void setup()
{
  // init serial
  Serial.begin(115200);
  // Initial delay to give chance to the com port to connect
  delay(2000);

  // thing init and send data
  //please create a gateway for every per node and set it here
#define gateway_id 4172
#define gateway_key "6869376AF0D54449C1C22ED5BBA2A18F"

#define ssid  "PLUSNET-7F63NX"
#define pass "4e3acd6a4f"

#define thing_id 37

  uint8_t thing_key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  IoTThing thing(ssid, pass, thing_id, thing_key, gateway_id, gateway_key);

  thing.register_ack(ack_callback);
  thing.register_timeout(timeout_callback);
  thing.init();

  uint8_t msg[IoTThing_MAX_MESSAGE_LEN];

  msg[0] = random(30); // e.g. temp
  msg[1] =  random(50, 90); // e.g. humidity

  // log what we send
  Serial.println("Sending data : ");
  for (int i = 0; i < 2; i++)
  {
    Serial.println(msg[i]);
  }

  thing.send(msg, 2);

  // wait for the data to be sent or timeout
  for (go_to_ulp = false; !go_to_ulp; )
  {
    thing.work();
    yield();
  }

  Serial.println("Go To ULP blink mode");

  rtc_gpio_init(LED_GPIO_NUM_PIN);
  rtc_gpio_set_direction(LED_GPIO_NUM_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);

  // led ON for 500msec, off for 4500msec , repeat 60 times
  blink_while_wait_ulp(400, 4500, 60); 
}


///////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP

void loop() 
{
}
