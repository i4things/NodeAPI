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
// WIFI


#if defined (ESP32)
#include <WiFi.h>
#endif

#if defined (ESP8266)
#include <ESP8266WiFi.h>
#endif

#pragma push_macro("LOG64_ENABLED")
//#undef LOG64_ENABLED

#define IoTGatewayWifi_MAX_PACKET_WAIT 1000
#define IoTGatewayWifi_WAIT_TIMEOUT  1000
#define IoTGatewayWifi_DEFAULT_TIMEOUT 1000
#define IoTGatewayWifi_EXTENDED_TIMEOUT 30000
#define IoTGatewayWifi_CONNECT_TIMEOUT_DEFAULT  60000
#define IoTGatewayWifi_BUF_MAX_SIZE 1024
#define IoTGateway_MAX_MESSAGE_LEN 51

#define IoTGatewayWifi_SERVER_PORT 5409
#define IoTGatewayWifi_SERVER_HOST "server.i4things.com"

class IoTGatewayWifiCallback
{
  public :
    virtual void received(uint8_t buf_[], uint8_t start_index_, uint8_t size_) = 0;
    virtual void data_sent(uint16_t size_) = 0;
    virtual void server_time(uint32_t time_) = 0;
};

class IoTGatewayWifi
{
  public :
    IoTGatewayWifi(const char * ssid_,
                   const char * pass_,
                   IoTGatewayWifiCallback * receive_callback_) :
      receive_callback(receive_callback_)
    {
      strcpy(ssid, ssid_);
      strcpy(pass, pass_);
    }

    // call before using
    void init()
    {
      WiFi.persistent (false);
      reset_last_execute = millis();

      response_wait = false;

      work_timeout = IoTGatewayWifi_DEFAULT_TIMEOUT;

      connect_timeout = 0;
      connect_last_execute = millis();
      work_last_execute = millis();

      queue_buf_size = 0;
      queue_buf_size_in_network = 0;
      decode_buf_size = 0;

      last_packet_time = 0;

      WiFi.mode(WIFI_STA);

      WiFi.setAutoConnect(false);
      //WiFi.setAutoReconnect(false);
#if defined (LOG64_ENABLED)
      LOG64_SET("Connecting to SSID[");
      LOG64_SET(ssid);
      LOG64_SET("] PASS[");
      LOG64_SET(pass);
      LOG64_SET("] <");
      LOG64_NEW_LINE;
#endif
      disconnect();
      WiFi.begin(ssid, pass);

#if defined (LOG64_ENABLED)
      LOG64_SET("WIFI: INIT");
      LOG64_NEW_LINE;
      LOG64_SET("Version: ");
      LOG64_SET(ESP.getSdkVersion());
      LOG64_NEW_LINE;
#endif
    }

    // set ssid and pass ( in case after start needs to be changed)
    void set_ssid_pass(const char * ssid_, const char * pass_)
    {
      strcpy(ssid, ssid_);
      strcpy(pass, pass_);
      flush();
      client.stop();
      disconnect();
      WiFi.begin(ssid, pass);
    }

    uint8_t signal_strength()
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        return 0;
      }
      int dBm = WiFi.RSSI();
      if (dBm <= -100)
      {
        return 0;
      }
      if (dBm >= -50)
      {
        return 100;
      }
      return 2 * (dBm + 100);
    }


    void send(uint8_t buf[], uint8_t size)
    {
      if ((queue_buf_size + ((uint16_t)size)) > IoTGatewayWifi_BUF_MAX_SIZE)
      {
#if defined (LOG64_ENABLED)
        LOG64_SET("WIFI: SEND_DATA ASYNC : NO SPACE : OLD DATA CLEARED");
        LOG64_NEW_LINE;
#endif
        queue_buf_size = 0;
      }

      memcpy((void *) & queue_buf[queue_buf_size], (void *) buf, size);
      queue_buf_size += ((uint16_t)size);
    }

  private:
    void flush()
    {
      // clear if there is some old response
      for (; client.available() > 0;)
      {
        client.read();
        yield();
      }
      client.flush();
    }

    void disconnect()
    {
      delay(10);

      WiFi.disconnect();
      WiFi.mode(WIFI_OFF); //workaround
      uint32_t my_start = millis();
      while ( ( WiFi.status() == WL_CONNECTED ) &&  ((((uint32_t)((uint32_t)millis()) - my_start)) < ((uint32_t)10000)) )
      {
        yield();
      }
      WiFi.mode(WIFI_STA);
    }

    void decode()
    {
      for (; client.available() > 0;)
      {
        decode_buf[decode_buf_size++] = client.read();
        if (decode_buf_size == 1)
        {
          if (decode_buf[0] > IoTGateway_MAX_MESSAGE_LEN)
          {
            //max length check
#if defined (LOG64_ENABLED)
            LOG64_SET("WIFI TRY DECODE : UP TO 51 BYTES FOR PACKET SIZE IS SUPPORTED - RESET CONNECTION");
            LOG64_NEW_LINE;
#endif
            //clear
            flush();
            client.stop();
            decode_buf_size = 0;
            return;
          }
        }

        // process response
        if (decode_buf_size == (decode_buf[0] + 1))
        {
          // all other will have size > 5
          if  (decode_buf_size == 5)
          {
            uint32_t server_time;
            ((uint8_t*)&server_time)[0] = decode_buf[1];
            ((uint8_t*)&server_time)[1] = decode_buf[2];
            ((uint8_t*)&server_time)[2] = decode_buf[3];
            ((uint8_t*)&server_time)[3] = decode_buf[4];
#if defined (LOG64_ENABLED)
            LOG64_SET("WIFI TRY DECODE : SERVER TIME[");
            LOG64_SET((uint32_t)server_time);
            LOG64_SET("]");
            LOG64_NEW_LINE;
#endif
           receive_callback->server_time(server_time);
          }
          else
          {
            receive_callback->received(decode_buf, 1, decode_buf_size - 1);
          }

          decode_buf_size = 0;

          // reset waiting - as we have recived at least one packet
          response_wait = false;
        }

        yield();
      }
    }

  public:

    void work()
    {

      decode();


      if (response_wait)
      {
        if (((uint32_t)(((uint32_t)millis()) - response_wait_execute)) >= IoTGatewayWifi_WAIT_TIMEOUT)
        {
          work_last_execute = millis();
#if defined (LOG64_ENABLED)
          LOG64_SET("WIFI: RESET CONNECTION : SYNC NOT RECEIVED");
          LOG64_NEW_LINE;
#endif
          //clear
          flush();
          client.stop();

          response_wait = false;

          work_last_execute = millis();

          return;
        }
      }

      if ( WiFi.status() != WL_CONNECTED)
      {
        if (((uint32_t)(((uint32_t)millis()) - connect_last_execute)) >= connect_timeout)
        {

          connect_last_execute = millis();
          connect_timeout = IoTGatewayWifi_CONNECT_TIMEOUT_DEFAULT;
#if defined (LOG64_ENABLED)
          LOG64_SET("WIFI : Schedule connecting to SSID[");
          LOG64_SET(ssid);
          LOG64_SET("] PASS[");
          LOG64_SET(pass);
          LOG64_SET("]");
          LOG64_NEW_LINE;
#endif

          disconnect();
          WiFi.begin(ssid, pass);
        }
      }
      else
      {

        connect_timeout = 0;
      }


      if ((((uint32_t)(((uint32_t)millis()) - work_last_execute)) >= work_timeout) && (!response_wait))
      {
        work_last_execute = millis();
        // make sure we are still connected to WIFI
        if ( WiFi.status() == WL_CONNECTED)
        {
          // check if the connection is established
          if (!client.connected())
          {
            //clear
            flush();
            client.stop();
#if defined (LOG64_ENABLED)
            LOG64_SET("WIFI : RECONNECT : TRY CONNECT TO SERVER...");
            LOG64_NEW_LINE;
#endif
            if (!client.connect(IoTGatewayWifi_SERVER_HOST, IoTGatewayWifi_SERVER_PORT))
            {
#if defined (LOG64_ENABLED)
              LOG64_SET("WIFI : RECONNECT : CAN'T CONNECT TO SERVER[");
              LOG64_SET(IoTGatewayWifi_SERVER_HOST);
              LOG64_SET(":");
              LOG64_SET(IoTGatewayWifi_SERVER_PORT);
              LOG64_SET("]");
              LOG64_NEW_LINE;
#endif
              response_wait = false;

              work_timeout = IoTGatewayWifi_EXTENDED_TIMEOUT;

              return;
            }
            else
            {
              work_timeout = IoTGatewayWifi_DEFAULT_TIMEOUT;
              client.setNoDelay(true);
#if defined (LOG64_ENABLED)
              LOG64_SET("WIFI : CONNECTED HOST:PORT[");
              LOG64_SET(IoTGatewayWifi_SERVER_HOST);
              LOG64_SET(":");
              LOG64_SET(IoTGatewayWifi_SERVER_PORT);
              LOG64_SET("]");
              LOG64_NEW_LINE;
#endif
              // clear
              flush();

              decode_buf_size = 0;
            }
          }
          else
          {
            // clear the in_network buffer
            queue_buf_size_in_network = 0;
          }

          if (queue_buf_size_in_network > 0)
          {
            // we still have something to send from previouse disconnect
            if ((queue_buf_size_in_network + queue_buf_size) <= IoTGatewayWifi_BUF_MAX_SIZE)
            {
              // we have space lets insert it
#if defined (LOG64_ENABLED)
              LOG64_SET("WIFI: MOVING IN NETWORK BUFFER BACK[");
              LOG64_SET((uint16_t)queue_buf_size_in_network);
              LOG64_SET("]");
              LOG64_NEW_LINE;
#endif
              memmove((void *) & queue_buf[queue_buf_size_in_network], (void *) queue_buf, queue_buf_size);
              memcpy((void *) queue_buf, (void *)queue_buf_in_network, queue_buf_size_in_network);
              queue_buf_size += queue_buf_size_in_network;;
            }
            else
            {
              // we have space lets insert it
#if defined (LOG64_ENABLED)
              LOG64_SET("WIFI: NO SPACE LEFT IN THE WIFI BUFFER - CLEANING NETWORK BUFFER");
              LOG64_NEW_LINE;
#endif
            }
            // clean netwrok buffer
            queue_buf_size_in_network = 0;
          }


          if (queue_buf_size > 0)
          {
            // version 2.3.0 with update for async
            //if (!client.isSendWaiting())

            // version 2.4.1
            // for ESP32 skip and try multiple retry manually
            {
              // copy current buffer to in_network buffer
              memcpy((void *)queue_buf_in_network, (void *) queue_buf, queue_buf_size);
              queue_buf_size_in_network = queue_buf_size;

              // try write
              bool write_not_ok = true;
              for (uint32_t write_start = (uint32_t)millis();;)
              {
                uint16_t res = client.write((const uint8_t *)queue_buf, queue_buf_size);
#if defined (LOG64_ENABLED)
                LOG64_SET("WIFI: BYTES WRITTEN[");
                LOG64_SET(res);
                LOG64_SET("]");
                LOG64_NEW_LINE;
#endif
                if (res == queue_buf_size)
                {
                  write_not_ok = false;
                  break;
                }
                else if (((uint32_t) (((uint32_t)millis()) -  write_start)) > IoTGatewayWifi_MAX_PACKET_WAIT )
                {
                  break;
                }

                // compact buffer and try again
                if (res > 0)
                {
                  memmove((void *) & queue_buf[res], (void *) queue_buf, res);
                  queue_buf_size -= res;
                }
                yield();
              }

              if (write_not_ok)
              {
#if defined (LOG64_ENABLED)
                LOG64_SET("WIFI: SEND DATA : BYTE CANNOT BE WRITTEN IN ONE GO MESSAGE SKIPPED");
                LOG64_NEW_LINE;
#endif
                //clear
                flush();
                client.stop();

              }
              else
              {
                response_wait = true;
                response_wait_execute = millis();
#if defined (LOG64_ENABLED)
                LOG64_SET("WIFI: SEND DATA :  SIZE[");
                LOG64_SET(queue_buf_size);
                LOG64_SET("] WAITING SYNC");
                LOG64_NEW_LINE;
#endif
                receive_callback->data_sent(queue_buf_size);
              }

              // clean buff
              queue_buf_size = 0;
            }
          }
        }
      }
    }

  private:
    // wait for response 10 sec
    bool response_wait;
    uint32_t response_wait_execute;


    // 1 sec
    uint32_t work_last_execute;
    uint32_t work_timeout;

    // 2 min
    uint32_t reset_last_execute;

    // 60 sec
    uint32_t connect_last_execute;
    uint32_t connect_timeout;


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //QUEUE SEND

    uint8_t queue_buf[IoTGatewayWifi_BUF_MAX_SIZE];
    uint16_t queue_buf_size;

    uint8_t queue_buf_in_network[IoTGatewayWifi_BUF_MAX_SIZE];
    uint16_t queue_buf_size_in_network;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //DECODE RECEIVE
    uint8_t decode_buf[IoTGatewayWifi_BUF_MAX_SIZE];
    uint16_t decode_buf_size;

    WiFiClient client;
    uint16_t last_packet_time;

    char ssid[33];
    char pass[65];

    IoTGatewayWifiCallback * receive_callback;
};


#pragma pop_macro("LOG64_ENABLED")
