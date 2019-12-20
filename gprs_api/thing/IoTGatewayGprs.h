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
// GPRS



#pragma push_macro("LOG64_ENABLED")
//#undef LOG64_ENABLED

#define GPRS_SERVER_PORT 5409
#define GPRS_SERVER_HOST "server.i4things.com"

#define GPRS_BAUD 19200

#define GPRS_MAX_BUF_SIZE 128

#define GPRS_MAX_RECV_BUF_SIZE 1025
#define GPRS_RECV_TIMEOUT 5000

//generic constants
#define GPRS_STATE_LEN 21

#define GPRS_STATUS_NA "---"
#define GPRS_RESPONSE_ANY "*"

// max required soft restarts without hard restart -1 e.g. 2 is 3 times soft restart
#define GPRS_SOFT_RESET_MAX 2

#define GPRS_SKIP_RESPONSE "+XDRVI:"

#include "IoTSim800l.h"

class IoTGatewayGprsCallback
{
  public :
    virtual void received(uint8_t buf_[], uint8_t start_index_, uint8_t size_) = 0;
    virtual void data_sent(uint16_t size_) = 0;
    virtual void server_time(uint32_t time_) = 0;
};

class IoTGatewayGprs
{
  public :
    IoTGatewayGprs(const char * apn_,
                   const char * user_,
                   const char * pass_,
                   uint8_t hardware_serial_num_,
                   uint8_t pin_rx_,
                   uint8_t pin_tx_,
                   uint8_t pin_reset_,
                   IoTGatewayGprsCallback * receive_callback_) :
      receive_callback(receive_callback_),
      GPRS_SERIAL(hardware_serial_num_),
      GPRS_PIN_RX(pin_rx_),
      GPRS_PIN_TX(pin_tx_),
      GPRS_PIN_RESET(pin_reset_),
      SERVER_PORT(GPRS_SERVER_PORT)
    {
      strcpy(GPRS_APN, apn_);
      strcpy(GPRS_USER, user_);
      strcpy(GPRS_PASS, pass_);

      strcpy(SERVER_HOST, GPRS_SERVER_HOST);

      GPRS_FIRST_HARDWARE_RESET = true;
      // init serial
      GPRS_DEFAULT_BAUD();
      // reset PIN init
      pinMode(GPRS_PIN_RESET, OUTPUT);
      digitalWrite(GPRS_PIN_RESET, HIGH);

      GPRS_SET_STATUS(GPRS_STATUS_NA, 0);

      GPRS_COMMAND_I = 2; // we do not want reset on first send
      GPRS_COMMAND_J = 0;

      GPRS_SIZE = 0;
      GPRS_IS_SERVER_IP = false;
      GPRS_IS_RECV_DATA = false;
      GPRS_RECV_DATA_SIZE = 0;
      GPRS_RECV_TEMP_SIZE = 0;
      GPRS_RECV_START_TIME = 0;
      GPRS_SIGNAL_DATA = false;

      GPRS_LAST_COMMAND[0] = 0;

      GPRS_OPERATION_START_TIME = 0;

      GPRS_QUEUE_BUF_SIZE = 0;
      GPRS_BUF_IN_NETWORK_SIZE = 0;

      GPRS_SOFT_RESET_COUNT = 0;

      GPRS_DO = false;

      GPRS_DNS_AT_INDEX = 14; // actual command and len not used in SIM800L case

      GPRS_RECEIVE_AT_INDEX = 18; // actual command and len not used in SIM800L case

      // exmaple for another module which requires DNS request first
      //            GPRS_DNS_AT_INDEX = 14;
      //            GPRS_DNS_AT = GPRS_COMMAND[GPRS_DNS_AT_INDEX][2][0];
      //            GPRS_DNS_AT_SIZE = strlen(GPRS_DNS_AT);
      //
      //            GPRS_RECEIVE_AT_INDEX = 18;
      //            GPRS_RECEIVE_AT = GPRS_COMMAND[GPRS_RECEIVE_AT_INDEX][1][0];
      //            GPRS_RECEIVE_AT_SIZE = strlen(GPRS_RECEIVE_AT);

      GPRS_TCPSEND_RESPONSE_INDEX = 16;
      GPRS_TCPSEND_RESPONSE = GPRS_COMMAND[GPRS_TCPSEND_RESPONSE_INDEX][1][0];
      GPRS_TCPSEND_RESPONSE_SIZE = strlen(GPRS_TCPSEND_RESPONSE);
      GPRS_SIGNAL_INDEX = 6;
      GPRS_SIGNAL_AT = GPRS_COMMAND[GPRS_SIGNAL_INDEX][1][0];;
      GPRS_SIGNAL_SIZE = strlen(GPRS_SIGNAL_AT);

      GPRS_INIT_INDEX = 1;

#if defined (LOG64_ENABLED)
      LOG64_SET(F("GPRS: INIT"));
      LOG64_NEW_LINE;
#endif


    }

    // call before using
    void init()
    {

    }

    // set apm user and pass ( in case after start needs to be changed)
    void set_apn_user_pass(const char * apn_, const char * user_, const char * pass_)
    {
      strcpy(GPRS_APN, apn_);
      strcpy(GPRS_USER, user_);
      strcpy(GPRS_PASS, pass_);
    }

    uint8_t signal_strength()
    {
      return GPRS_CSQ_STRENGTH_PERCENT;
    }


    void send(uint8_t buf[], uint8_t size)
    {
      // we need one byte for 13 at the end for the GPRS model protocol
      if ((GPRS_QUEUE_BUF_SIZE + ((uint16_t)size)) >= (GPRS_MAX_BUF_SIZE - 1))
      {
#if defined (LOG64_ENABLED)
        LOG64_SET("GPRS: SEND_DATA ASYNC : NO SPACE : OLD DATA CLEARED");
        LOG64_NEW_LINE;
#endif
        GPRS_QUEUE_BUF_SIZE = 0;
      }

      memcpy((void *) & GPRS_QUEUE_BUF[GPRS_QUEUE_BUF_SIZE], (void *) buf, size);
      GPRS_QUEUE_BUF_SIZE += ((uint16_t)size);
    }

  private:

    // special DATA response handling
    inline bool GPRS_PROCESS_RECEIVE_DATA(char c, char data[], uint16_t & size, uint16_t & recv_data_size, uint16_t & recv_temp_size, uint32_t & recv_start_time)
    {
      if (recv_data_size != 0)
      {
        // receiving is already started - we can check for timeout
        if (((uint32_t)(((uint32_t)millis()) - recv_start_time)) >= GPRS_RECV_TIMEOUT)
        {
          // all that we can is received ( data waiting timeout passed)
          // temp size hold last know good buffer size
          size = recv_temp_size;
          recv_data_size = recv_temp_size;
          return true;
        }
      }
      // in case of SIM800L we wait a timeout before starting reading - and read all available and assemble it here
      if (recv_data_size == 0)
      {
        // we still read the expected size

        // for SIM800l first one is 0A(enter) - skip
        // we wait for next byte to knwio the size
        recv_data_size = 1;
        recv_temp_size = 0;
      }
      else
      {
        // assemble data
        data[size++] = c;
        // if all expected data in this stage is read
        if (size == recv_data_size)
        {
          if ((recv_data_size - recv_temp_size) == 1)
          {
            // this is packet size byte
            recv_data_size += (uint16_t)c;
          }
          else
          {
            // old packet is done we are staring new packet
            recv_temp_size = recv_data_size;
            recv_data_size++;
          }
        }
      }

      return false;
    }

    // special SIGNAL response handling
    inline bool GPRS_PROCESS_SIGNAL(char c, char data[], uint16_t & size)
    {
      bool ret = false;
      if ((c == '\n') || (c == '\r'))
      {
        // end found
        ret = true;
      }
      if (c == ',')
      {
        data[size++] = 0;
        // remove prefix
        memmove(data, &data[GPRS_SIGNAL_SIZE], size - GPRS_SIGNAL_SIZE + 1);
        size -= GPRS_SIGNAL_SIZE;
      }
      // skip if we have assembled the signal already - size cannot be 0 - as there response is prefix always before entering this fuction for first time
      else if (data[size - 1] != 0)
      {
        data[size++] = c;
      }

      return ret;
    }


    // special DNS response handling
    inline bool GPRS_PROCESS_SERVER_IP(char c, char data[], uint16_t & size, bool & is_server_ip)
    {
      bool ret = false;
      // check if first char is number - if not - this is not a  server_ip but +DNS:OK text
      if ((GPRS_DNS_AT_SIZE == size) && ((c < '0') || (c > '9') ))
      {
        // this is a +DNS:OK
        data[size++] = c;
        is_server_ip = false;
      }
      else
      {
        if ((c == '\n') || (c == '\r'))
        {
          data[size++] = 0;
          // remove the prefix
          memmove(data, &data[GPRS_DNS_AT_SIZE], size - GPRS_DNS_AT_SIZE + 1);
          ret = true;
        }
        else
        {
          data[size++] = c;
        }
      }

      return ret;
    }

    // add new char from serial
    // if return true then a line is assembled in ret
    // size will be 0 when return true - end of string is by trailing 0
    //
    // empty lines are omitted
    //
    // to process when true is returned - first check the is_server_ip - if true then the data is server_ip is in \0 terminated string
    // next check recv_data_size - if recv_data_size > 0 then data is receive data
    // next the data is just a  response in \0 terminated string
    // after processing the server_ip or receive_data or signal the flags and sizes need to be cleared
    inline bool GPRS_ADD(char c, char data[], uint16_t & size,
                         bool & is_server_ip,
                         bool & is_recv_data,
                         uint16_t & recv_data_size,
                         uint16_t & recv_temp_size,
                         uint32_t & recv_start_time,
                         bool & signal_data)
    {
      bool ret = false;

      // check for buf overfill
      if (size >= (GPRS_MAX_BUF_SIZE - 1))
      {
        // no space give what we have clear buff and continue
        data[size++] = 0;
        ret = true;
      }
      else
      {
        // process byte/char
        if (signal_data)
        {
          if (GPRS_PROCESS_SIGNAL(c, data, size))
          {
            // all is filled inside the function
            ret = true;
          }
        } else if (is_server_ip)
        {
          if (GPRS_PROCESS_SERVER_IP(c, data, size, is_server_ip))
          {
            // all is filled inside the function
            ret = true;
          }
        } else if (is_recv_data)
        {
          if (GPRS_PROCESS_RECEIVE_DATA(c, data, size, recv_data_size, recv_temp_size, recv_start_time))
          {
            // all is filled inside the function
            ret = true;
          }
        }
        else if ((c == '\n') || (c == '\r'))
        {
          data[size++] = 0;
          ret = true;
        }
        else
        {
          data[size++] = c;
          if ((GPRS_TCPSEND_RESPONSE_SIZE == size) && (memcmp(GPRS_TCPSEND_RESPONSE, data, GPRS_TCPSEND_RESPONSE_SIZE) == 0))
          {
            // this is respone to TCPSEND - simulate end of line
            data[size++] = 0;
            ret = true;
          }
        }

        if ((!is_server_ip) && (!is_recv_data) && (!signal_data))
        {
          if ((GPRS_SIGNAL_SIZE == size) && (memcmp(GPRS_SIGNAL_AT, data, GPRS_SIGNAL_SIZE) == 0))
          {
            // special case signal data
            signal_data = true;
          }
          //no need for DNS query in SIM800L case and prefix for rceivingg data
        }
      }

      if (ret)
      {
        // omit empty lines
        if (size <= 1)
        {
          ret = false;
        }
        size = 0;
      }

      return ret;
    }

    // get a value for replacment for command
    // if return false do not add trailing 0 string - e.g. it is data
    inline bool GPRS_GET(char c[], char ret[], uint16_t & size)
    {
      size = 0;
      bool res = true;

      if (strcmp("APN", c) == 0)
      {
        strcpy(ret, GPRS_APN);
        size = strlen(GPRS_APN);
      }
      else if (strcmp("USER", c) == 0)
      {
        strcpy(ret, GPRS_USER);
        size = strlen(GPRS_USER);
      }
      else if (strcmp("PASS", c) == 0)
      {
        strcpy(ret, GPRS_PASS);
        size = strlen(GPRS_PASS);
      }
      else if (strcmp("SERVER_HOST", c) == 0)
      {
        strcpy(ret, SERVER_HOST);
        size = strlen(SERVER_HOST);
      }
      else if (strcmp("SERVER_PORT", c) == 0)
      {
        itoa ((int)SERVER_PORT, ret, 10);
        size = strlen(ret);
      }
      else if (strcmp("SERVER_IP", c) == 0)
      {
        strcpy(ret, String(GPRS_SERVER_IP).c_str());
        size = strlen(String(GPRS_SERVER_IP).c_str());
      }
      else if (strcmp("DATA_LEN", c) == 0)
      {
        if ((GPRS_BUF_IN_NETWORK_SIZE > 0) && ((GPRS_BUF_IN_NETWORK_SIZE + GPRS_QUEUE_BUF_SIZE) < (GPRS_MAX_BUF_SIZE - 1)))
        {
          // we still have something to send from previouse disconnect
          // we have space lets insert it
#if defined (LOG64_ENABLED)
          LOG64_SET("GPRS: ADDING TO NETWORK BUFFER[");
          LOG64_SET((uint16_t)GPRS_QUEUE_BUF_SIZE);
          LOG64_SET("]");
          LOG64_NEW_LINE;
#endif

          memcpy((void *) & GPRS_BUF_IN_NETWORK[GPRS_BUF_IN_NETWORK_SIZE], (void *) GPRS_QUEUE_BUF, GPRS_QUEUE_BUF_SIZE);
          GPRS_BUF_IN_NETWORK_SIZE +=  GPRS_QUEUE_BUF_SIZE;
        }
        else
        {
          // we either do not have space to insert it or network buffer is empty

          memcpy((void *) GPRS_BUF_IN_NETWORK, (void *) GPRS_QUEUE_BUF, GPRS_QUEUE_BUF_SIZE);
          GPRS_BUF_IN_NETWORK_SIZE = GPRS_QUEUE_BUF_SIZE;
        }
        GPRS_QUEUE_BUF_SIZE = 0;

        itoa (GPRS_BUF_IN_NETWORK_SIZE, ret, 10);
        size = strlen(ret);

#if defined (LOG64_ENABLED)
        LOG64_SET("GPRS: DATA PREPARED LEN[");
        LOG64_SET((const char *)ret);
        LOG64_SET("]");
        LOG64_NEW_LINE;
#endif
      }
      else if (strcmp("DATA", c) == 0)
      {

        memcpy((void *) ret, (void *) GPRS_BUF_IN_NETWORK, GPRS_BUF_IN_NETWORK_SIZE);
        size = GPRS_BUF_IN_NETWORK_SIZE;
        //for SIM800L no need to add 0x0D at teh end
#if defined (LOG64_ENABLED)
        LOG64_SET("GPRS: SEND DATA :  SIZE[");
        LOG64_SET(GPRS_BUF_IN_NETWORK_SIZE);
        LOG64_SET("] WAITING SYNC");
        LOG64_NEW_LINE;
#endif

        res = false;
      }

      return res;
    }

    // fill/replace command with value
    inline void GPRS_FILL(const char in[], char ret[], uint16_t & size)
    {
      char c[32];
      uint16_t k = 0;

      bool f = false;

      size = 0;

      bool is_fi_str = true;

      for (uint16_t i = 0; in[i] != 0; i++)
      {
        if (in[i] == '@')
        {
          if (f)
          {
            c[k] = 0;
            // exchange
            char fi[GPRS_MAX_BUF_SIZE];
            uint16_t fi_size;
            is_fi_str = GPRS_GET(c, fi, fi_size);
            for (uint16_t l = 0; l < fi_size; l++)
            {
              ret[size++] = fi[l];
            }
            k = 0;
            f = false;
          }
          else
          {
            f = true;
          }
        }
        else
        {
          if (f)
          {
            c[k++] = in[i];
          }
          else
          {
            ret[size++] = in[i];
          }
        }

      }

      if (is_fi_str)
      {
        ret[size++] = '\r';
      }
    }

    // next command will skip reset
    inline bool GPRS_MOVE_TO_NEXT_COMMAND()
    {
      bool ret = false;

      GPRS_COMMAND_I++;
      if (GPRS_COMMAND_I >= GPRS_STATE_LEN)
      {
        // clean soft reset count
        GPRS_SOFT_RESET_COUNT = 0;

        GPRS_COMMAND_I = 2;
        GPRS_IS_RECV_DATA = false;
        GPRS_DO = false;

        ret = true;
#if defined (LOG64_ENABLED)
        LOG64_SET(F("GPRS: WORKER STOP"));
        LOG64_NEW_LINE;
#endif
      }
      // we need to check for index 2 and 3 as in case of timeout hit on last command index will be 3
      else if (((GPRS_COMMAND_I == 2) || (GPRS_COMMAND_I == 3)) && (GPRS_QUEUE_BUF_SIZE == 0) && (GPRS_BUF_IN_NETWORK_SIZE == 0))
      {
        // clean soft reset count
        GPRS_SOFT_RESET_COUNT = 0;

        GPRS_COMMAND_I = 2;
        GPRS_IS_RECV_DATA = false;
        GPRS_DO = false;

        ret = true;

        LOG64_SET(F("GPRS: WORKER STOP - NO DATA TO BE SENT."));
        LOG64_NEW_LINE;
      }

      GPRS_COMMAND_J = 0;

      // for SIM800l special case - data is without prefix
      if (GPRS_COMMAND_I == GPRS_RECEIVE_AT_INDEX)
      {
        // special case data waited without prefix
        GPRS_IS_RECV_DATA = true;
        GPRS_RECV_START_TIME = millis();
        GPRS_RECV_DATA_SIZE = 0;
        GPRS_RECV_TEMP_SIZE = 0;

      }

      return ret;
    }

    inline void GPRS_EXECUTE_COMMAND(bool from_retry)
    {
      for (;;)
      {
        if (GPRS_COMMAND_J == 0)
        {
          if (GPRS_COMMAND_I == 0)
          {
            GPRS_SOFT_RESET_COUNT++;
          }

          if (GPRS_COMMAND[GPRS_COMMAND_I][0][0] != NULL)
          {
            char fi[GPRS_MAX_BUF_SIZE];
            uint16_t fi_s = 0;
            GPRS_FILL(GPRS_COMMAND[GPRS_COMMAND_I][0][0], fi, fi_s);

            // store last command for echo check
            memcpy(GPRS_LAST_COMMAND, fi, fi_s);
            // make sure we skip the trailing \r
            if ((fi_s > 0) && (GPRS_LAST_COMMAND[fi_s - 1] == '\r'))
            {
              GPRS_LAST_COMMAND[fi_s - 1] = 0;
            }
            else
            {
              GPRS_LAST_COMMAND[fi_s] = 0;
            }

            GPRS_SERIAL.write((const uint8_t *)fi, fi_s);
            GPRS_OPERATION_START_TIME = millis();
            if (!from_retry)
            {
              GPRS_OPERATION_RETRY = GPRS_COMMAND_RETRY[GPRS_COMMAND_I];
            }
#if defined (LOG64_ENABLED)
            LOG64_SET(F("GPRS: COMMAND["));
            if (GPRS_COMMAND_I == GPRS_RECEIVE_AT_INDEX)
            {
              for (int i = 0; i < fi_s; i++)
              {
                LOG64_SET(String((uint8_t)fi[i]));
              }
            }
            else
            {
#endif
              fi[(fi_s == 0) ? 0 :  fi_s - 1] = 0;
#if defined (LOG64_ENABLED)
              LOG64_SET(fi);
            }
            LOG64_SET(F("] SIZE["));
            LOG64_SET(fi_s);
            LOG64_SET(F("]"));
            LOG64_NEW_LINE;
#endif
          }
          else
          {
            GPRS_LAST_COMMAND[0] = 0;
          }
        }

        // for SIM800l
        // we cannot move as we are still waiting for data
        // the SIM800l AT logic do not have prefix before data received from the server
        if (!GPRS_IS_RECV_DATA)
        {
          GPRS_COMMAND_J++;
        }
        else
        {
          // get out and wait for data from server
          break;
        }

        if (GPRS_COMMAND_J >= 4)
        {
          if (GPRS_MOVE_TO_NEXT_COMMAND())
          {
            // finished for now
            break;
          }
          continue;
        }

        if (GPRS_COMMAND[GPRS_COMMAND_I][GPRS_COMMAND_J][0] == NULL)
        {
          if (GPRS_MOVE_TO_NEXT_COMMAND())
          {
            // finished for now
            break;
          }
          continue;
        }
        break;
      }
    }

    inline bool GPRS_IS_EMPTY(char response[])
    {
      bool ret = true;
      for (int i = 0; i < GPRS_MAX_BUF_SIZE; i++)
      {
        // ommit only spaces
        if (response[i] > 32)
        {
          ret = false;
          break;
        }
        if (response[i] == 0)
        {
          break;
        }
      }

      return ret;
    }


    inline bool GPRS_IS_NOISE(char response[],
                              char skip_response[])
    {

      if (strstr(response, skip_response) != NULL)
      {
        return true;
      }

      bool ret = false;
      for (int i = 0; i < GPRS_MAX_BUF_SIZE; i++)
      {
        if (response[i] == 0)
        {
          break;
        }

        if (response[i] < 10)
        {
          ret = true;
          break;
        }

        if (response[i] > 126)
        {
          ret = true;
          break;
        }
      }

      return ret;
    }

    inline void GPRS_HARD_RESET(bool too_many_soft_reset)
    {
      if (too_many_soft_reset)
      {
#if defined (LOG64_ENABLED)
        LOG64_SET(F("GPRS: TOO MANY SOFT RESET["));
        LOG64_SET((uint16_t)GPRS_SOFT_RESET_COUNT);
        LOG64_SET(F("] - HARDWARE RESET"));
        LOG64_NEW_LINE;
#endif
      }
      // cleanup and start from beggining with hardware reset
      GPRS_COMMAND_I = 2; // we do not want soft reset after hard reset
      GPRS_IS_RECV_DATA = false;
      GPRS_COMMAND_J = 0;
      GPRS_INIT_INDEX = 1;
      GPRS_DO = false;

      GPRS_SOFT_RESET_COUNT = 0;
    }

    inline const char * GPRS_CSQ_TO_STRENGTH(uint16_t csq)
    {
      uint8_t s;

      if (csq == 99)
      {
        s = 0;
      }
      else
      {
        s = csq * 3;
      }

      if (s > 100)
      {
        s = 100;
      }

      itoa(s, GPRS_CSQ_STRENGTH, 10);
      strcat (GPRS_CSQ_STRENGTH, "%");

      return GPRS_CSQ_STRENGTH;
    }

    inline const uint8_t GPRS_CSQ_TO_STRENGTH_PERCENT(uint16_t csq)
    {
      uint8_t s;

      if (csq == 99)
      {
        s = 0;
      }
      else
      {
        s = csq * 3;
      }

      if (s > 100)
      {
        s = 100;
      }

      return s;
    }

    inline const char * GPRS_GET_STATUS()
    {
      return GPRS_STATUS;
    }
    inline void GPRS_SET_STATUS(const char * new_status, uint8_t signal_percent)
    {
      GPRS_CSQ_STRENGTH_PERCENT = signal_percent;
      GPRS_STATUS = new_status;
#if defined (LOG64_ENABLED)
      LOG64_SET(F("GPRS: STATUS ["));
      LOG64_SET(GPRS_STATUS);
      LOG64_SET(F("]"));
      LOG64_NEW_LINE;
#endif
    }

    inline void GPRS_NEW_BAUD(uint32_t baud)
    {
      // version 1
      GPRS_SERIAL.updateBaudRate(baud);

      // version 2
      //GPRS_SERIAL.begin(baud, SERIAL_8N1, GPRS_PIN_RX, GPRS_PIN_TX);
    }

    inline void GPRS_CLEAR_BAUD()
    {
      // version 1
      // nothing when special GPRS_NEW_BAUD(9600) is used

      // version 2
      //GPRS_SERIAL.end();
    }

    inline void GPRS_DEFAULT_BAUD()
    {
      GPRS_SERIAL.begin(GPRS_BAUD, SERIAL_8N1, GPRS_PIN_RX, GPRS_PIN_TX);
    }

    // if return false means init is done
    inline bool GPRS_INIT_DO_WORK()
    {
      switch (GPRS_INIT_INDEX)
      {
        case 0 :
          return false; // finished

        //////////////////////////////////////////////////////////////////////////////////////
        ////// start/stop/reset
        case 1 : // set disconnect
          {
            if (GPRS_FIRST_HARDWARE_RESET)
            {
              // fist start we want to keep it in HIGH - to not poerform swith-of and on cycle
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS INITIAL START WAIT 60sec"));
              LOG64_NEW_LINE;
#endif
              GPRS_FIRST_HARDWARE_RESET = false;
            }
            else
            {
              digitalWrite(GPRS_PIN_RESET, LOW);
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS DISCONNECT VCC DOWN AND WAIT 60sec"));
              LOG64_NEW_LINE;
#endif
            }
            GPRS_INIT_TIME = millis();
            GPRS_INIT_INDEX++;

            GPRS_CLEAR_BAUD();

          }
          return true;

        case 2 :  // wait 60000ms and connect to VCC
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 60000)
            {
              // init the serial
              GPRS_NEW_BAUD(GPRS_BAUD);
              // timeout passed set in low move to next
#if defined (LOG64_ENABLED)
              LOG64_SET(F("VCC SET CONNECT TO VCC - HARDWARE RESET AND - WAIT 60sec"));
              LOG64_NEW_LINE;
#endif
              digitalWrite(GPRS_PIN_RESET, HIGH);
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS VCC UP"));
              LOG64_NEW_LINE;
#endif
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;

            }
          }
          return true;
        case 3 :  // wait 60000ms for GPRS to start
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 60000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        //////////////////////////////////////////////////////////////////////////////////////
        ////// try 9600
        case 4 :
#if defined (LOG64_ENABLED)
          LOG64_SET(F("9600"));
          LOG64_NEW_LINE;
#endif
          GPRS_NEW_BAUD(9600);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 5 :  // wait 2000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              // timeout passed move to next
              GPRS_SERIAL.print(String("AT+IPR=") + String(GPRS_BAUD) + String("\r"));
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 6 :  // wait 2000ms read all from serial
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              for (; GPRS_SERIAL.available();)
              {
                GPRS_SERIAL.read();
              }
              GPRS_CLEAR_BAUD();
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 7 :  // wait 1000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        //////////////////////////////////////////////////////////////////////////////////////
        ////// try 14400
        case 8 :
#if defined (LOG64_ENABLED)
          LOG64_SET(F("14400"));
          LOG64_NEW_LINE;
#endif
          GPRS_NEW_BAUD(14400);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 9 :  // wait 2000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              // timeout passed move to next
              GPRS_SERIAL.print(String("AT+IPR=") + String(GPRS_BAUD) + String("\r"));
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 10 :  // wait 2000ms read all from serial
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              for (; GPRS_SERIAL.available();)
              {
                GPRS_SERIAL.read();
              }
              GPRS_CLEAR_BAUD();
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 11 :  // wait 1000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;

        //////////////////////////////////////////////////////////////////////////////////////
        ////// try 19200
        case 12 :
#if defined (LOG64_ENABLED)
          LOG64_SET(F("19200"));
          LOG64_NEW_LINE;
#endif
          GPRS_NEW_BAUD(19200);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 13 :  // wait 2000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              // timeout passed move to next
              GPRS_SERIAL.print(String("AT+IPR=") + String(GPRS_BAUD) + String("\r"));
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 14 :  // wait 2000ms read all from serial
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              for (; GPRS_SERIAL.available();)
              {
                GPRS_SERIAL.read();
              }
              GPRS_CLEAR_BAUD();
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 15 :  // wait 1000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        //////////////////////////////////////////////////////////////////////////////////////
        ////// try 28800
        case 16 :
#if defined (LOG64_ENABLED)
          LOG64_SET(F("28800"));
          LOG64_NEW_LINE;
#endif
          GPRS_NEW_BAUD(28800);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 17 :  // wait 2000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              // timeout passed move to next
              GPRS_SERIAL.print(String("AT+IPR=") + String(GPRS_BAUD) + String("\r"));
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 18 :  // wait 2000ms read all from serial
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              for (; GPRS_SERIAL.available();)
              {
                GPRS_SERIAL.read();
              }
              GPRS_CLEAR_BAUD();
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 19 :  // wait 1000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        //////////////////////////////////////////////////////////////////////////////////////
        ////// try 38400
        case 20 :
#if defined (LOG64_ENABLED)
          LOG64_SET(F("38400"));
          LOG64_NEW_LINE;
#endif
          GPRS_NEW_BAUD(38400);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 21 :  // wait 2000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              // timeout passed move to next
              GPRS_SERIAL.print(String("AT+IPR=") + String(GPRS_BAUD) + String("\r"));
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 22 :  // wait 2000ms read all from serial
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              for (; GPRS_SERIAL.available();)
              {
                GPRS_SERIAL.read();
              }
              GPRS_CLEAR_BAUD();
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 23 :  // wait 1000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        //////////////////////////////////////////////////////////////////////////////////////
        ////// try 57600
        case 24 :
#if defined (LOG64_ENABLED)
          LOG64_SET(F("57600"));
          LOG64_NEW_LINE;
#endif
          GPRS_NEW_BAUD(57600);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 25 :  // wait 2000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              // timeout passed move to next
              GPRS_SERIAL.print(String("AT+IPR=") + String(GPRS_BAUD) + String("\r"));
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 26 :  // wait 2000ms read all from serial
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              for (; GPRS_SERIAL.available();)
              {
                GPRS_SERIAL.read();
              }
              GPRS_CLEAR_BAUD();
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 27 :  // wait 1000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        //////////////////////////////////////////////////////////////////////////////////////
        ////// try 115200
        case 28 :
#if defined (LOG64_ENABLED)
          LOG64_SET(F("115200"));
          LOG64_NEW_LINE;
#endif
          GPRS_NEW_BAUD(115200);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 29 :  // wait 2000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              // timeout passed move to next
              GPRS_SERIAL.print(String("AT+IPR=") + String(GPRS_BAUD) + String("\r"));
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 30 :  // wait 2000ms read all from serial
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
            {
              for (; GPRS_SERIAL.available();)
              {
                GPRS_SERIAL.read();
              }
              GPRS_CLEAR_BAUD();
              GPRS_INIT_TIME = millis();
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        case 31 :  // wait 1000ms
          {
            if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
            {
              // timeout passed move to next
              GPRS_INIT_INDEX++;
            }
          }
          return true;
        //////////////////////////////////////////////////////////////////////////////////////
        ////// INIT AND ECHO OFF
        case 32 :
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 33 :  // wait 2000ms
          if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
          {
            // timeout passed move to next
            GPRS_INIT_INDEX++;
          }
          return true;
        case 34 : // init serial
          GPRS_NEW_BAUD(GPRS_BAUD);
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 35 : // set time
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 36 :  // wait 2000ms
          if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
          {
            // timeout passed move to next
            GPRS_INIT_INDEX++;
          }
          return true;
        case 37 : // send AT echo off and set time
#if defined (LOG64_ENABLED)
          LOG64_SET(F("GPRS: ECHO OFF"));
          LOG64_NEW_LINE;
#endif
          GPRS_SERIAL.print("ATE0\r");
          //GPRS_SERIAL.print("ATE0\r");
          GPRS_INIT_TIME = millis();
          GPRS_INIT_INDEX++;
          return true;
        case 38 :  // wait 2000ms and read all and send AT+CMEE=1/AT+CFUN=1 and set time
          if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
          {
            // timeout passed read all move to next
            for (; GPRS_SERIAL.available();)
            {
              uint8_t b = GPRS_SERIAL.read();
#if defined (LOG64_ENABLED)
              LOG64_SET(String((char)b));
#endif
            }
#if defined (LOG64_ENABLED)
            LOG64_NEW_LINE;
            LOG64_SET(F("GPRS: AT+CMEE=2"));
            LOG64_NEW_LINE;
#endif
            GPRS_SERIAL.print("AT+CMEE=2\r");
            GPRS_INIT_TIME = millis();
            GPRS_INIT_INDEX++;
          }
          return true;
        case 39 : // wait 2000ms and read all
          if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 2000)
          {
            // timeout passed read all move to next
            for (; GPRS_SERIAL.available();)
            {
              uint8_t b = GPRS_SERIAL.read();
#if defined (LOG64_ENABLED)
              LOG64_SET(String((char)b));
#endif
            }
#if defined (LOG64_ENABLED)
            LOG64_NEW_LINE;
#endif
            GPRS_INIT_TIME = millis();
            GPRS_INIT_INDEX++;
          }
          return true;
        case 40 :  // wait 1000ms
          if (((uint32_t)(((uint32_t)millis()) - GPRS_INIT_TIME)) >= 1000)
          {
            // timeout passed move to next
            GPRS_INIT_INDEX++;
          }
          return true;
        case 41 :  // finished
#if defined (LOG64_ENABLED)
          LOG64_SET(F("GPRS: RESET"));
          LOG64_NEW_LINE;
#endif
          {
            GPRS_INIT_INDEX = 0;
          }
          return true;


      }
    }
  public:
    void work()
    {
      if (GPRS_INIT_DO_WORK())
      {
        return;
      }

      // check if we need to start worker
      if ((!GPRS_DO) && (GPRS_COMMAND_I == 2) && ((GPRS_QUEUE_BUF_SIZE > 0) || (GPRS_BUF_IN_NETWORK_SIZE > 0)))
      {
        GPRS_DO = true;
#if defined (LOG64_ENABLED)
        LOG64_SET(F("GPRS: WORKER START"));
        LOG64_NEW_LINE;
#endif
        // read/clear all old data from serial
        for (; GPRS_SERIAL.available();)
        {
          GPRS_SERIAL.read();
        }
        GPRS_EXECUTE_COMMAND(false);
      }



      if (GPRS_DO)
      {

        // skip all next if timeout after command execution hasn't passed
        if (((uint32_t)(((uint32_t)millis()) - GPRS_OPERATION_START_TIME)) < GPRS_TIMEOUT_BEFORE_RESULT[GPRS_COMMAND_I])
        {
          return;
        }

        if (((uint32_t)(((uint32_t)millis()) - GPRS_OPERATION_START_TIME)) >= GPRS_TIMEOUT[GPRS_COMMAND_I])
        {


          // check for last
          if (GPRS_COMMAND_I == (GPRS_STATE_LEN - 1))
          {
            // last no need for full reset
#if defined (LOG64_ENABLED)
            LOG64_SET(F("GPRS: TIMEOUT HIT ON LAST COMMAND - MOVING TO FIRST WITHOUT RESET"));
            LOG64_NEW_LINE;
#endif
            GPRS_COMMAND_I = 2;
            GPRS_IS_RECV_DATA = false;
          }
          else
          {
#if defined (LOG64_ENABLED)
            LOG64_SET(F("GPRS: TIMEOUT HIT - RESET"));
            LOG64_NEW_LINE;
#endif

            if (GPRS_COMMAND_I <= GPRS_SIGNAL_INDEX)
            {
              GPRS_SET_STATUS(GPRS_STATUS_NA, 0);
            }

            if (GPRS_COMMAND_I == 0)
            {
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS: TIMEOUT HIT ON RESET - HARDWARE RESET"));
              LOG64_NEW_LINE;
#endif
              GPRS_HARD_RESET(false);

              // skip everything else and return
              return;
            }

            GPRS_COMMAND_I = 0;
            GPRS_IS_RECV_DATA = false;
            // check if too many soft reset and need of hard reset
            if (GPRS_SOFT_RESET_COUNT > GPRS_SOFT_RESET_MAX)
            {
              GPRS_HARD_RESET(true);

              // skip everything else and return
              return;
            }
          }

          GPRS_COMMAND_J = 0;


          // read all from serial
          for (; GPRS_SERIAL.available();)
          {
            GPRS_SERIAL.read();
          }

          GPRS_EXECUTE_COMMAND(false);

        }


        // check if we need to simulate data arrived from serial to enable the receive data wait to finish
        bool data_sym_on = GPRS_IS_RECV_DATA && (((uint32_t)(((uint32_t)millis()) - GPRS_RECV_START_TIME)) >= GPRS_RECV_TIMEOUT);

        if (GPRS_SERIAL.available() || data_sym_on)
        {
          char c;
          if (data_sym_on)
          {
            c = 'X';
          }
          else
          {
            c = GPRS_SERIAL.read();
          }
#if defined (LOG64_ENABLED)
          //          //debug purpose logging
          //          LOG64_SET(F("["));
          //          if (c <= 32)
          //          {
          //            LOG64_SET((uint8_t)c);
          //          }
          //          else
          //          {
          //            char cc[2];
          //            cc[0] = c;
          //            cc[1] = 0;
          //            LOG64_SET(cc);
          //          }
          //          LOG64_SET((uint8_t)c);
          //          LOG64_SET(F("]"));
#endif
          if (GPRS_ADD(c, GPRS_DATA, GPRS_SIZE,
                       GPRS_IS_SERVER_IP,
                       GPRS_IS_RECV_DATA,
                       GPRS_RECV_DATA_SIZE,
                       GPRS_RECV_TEMP_SIZE,
                       GPRS_RECV_START_TIME,
                       GPRS_SIGNAL_DATA))
          {
            //response is assembled
            if (GPRS_IS_SERVER_IP)
            {
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS: SERVER IP["));
              LOG64_SET(GPRS_DATA);
              LOG64_SET(F("]"));
              LOG64_NEW_LINE;
#endif
              // DNS response
              strcpy(GPRS_SERVER_IP, GPRS_DATA);
              GPRS_IS_SERVER_IP = false;

              GPRS_EXECUTE_COMMAND(false);
            }
            else if (GPRS_SIGNAL_DATA)
            {
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS: SIGNAL["));
              LOG64_SET(GPRS_DATA);
              LOG64_SET(F("]"));
              LOG64_NEW_LINE;
#endif
              uint16_t csq = atoi(GPRS_DATA);
              GPRS_SET_STATUS(GPRS_CSQ_TO_STRENGTH(csq), GPRS_CSQ_TO_STRENGTH_PERCENT(csq));

              GPRS_SIGNAL_DATA = false;

              GPRS_EXECUTE_COMMAND(false);
            }
            else if (GPRS_IS_RECV_DATA)
            {
              // all data arrived we can clean the send buffer
			  GPRS_BUF_IN_NETWORK_SIZE = 0;
			  
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS: DATA["));
              for (uint16_t i = 0; i < GPRS_RECV_DATA_SIZE; i++)
              {
                LOG64_SET((uint8_t)GPRS_DATA[i]);
              }
              LOG64_SET(",");
              LOG64_SET(GPRS_RECV_DATA_SIZE);
              LOG64_SET(F("]"));
              LOG64_NEW_LINE;
#endif
              // all data sedn and resonse received
              receive_callback->data_sent(0);
              //process response(s)
              uint16_t start_index = 0;
              uint16_t end_index;
              uint8_t tmp_buf[GPRS_RECV_DATA_SIZE];
              for ( uint16_t i  = 0; i <= GPRS_RECV_DATA_SIZE;)
              {
                if (start_index  == 0)
                {
                  //collect size and start
                  start_index = i + 1;
                  end_index = start_index + ((uint8_t)GPRS_DATA[i]);
                  i++;
                }
                else if (end_index == i)
                {
                  // packet assembled
                  uint8_t size = end_index - start_index;
                  if  (size == 4)
                  {
                    uint32_t server_time;
                    ((uint8_t*)&server_time)[0] = tmp_buf[0];
                    ((uint8_t*)&server_time)[1] = tmp_buf[1];
                    ((uint8_t*)&server_time)[2] = tmp_buf[2];
                    ((uint8_t*)&server_time)[3] = tmp_buf[3];
#if defined (LOG64_ENABLED)
                    LOG64_SET("GPRS: SERVER TIME[");
                    LOG64_SET((uint32_t)server_time);
                    LOG64_SET("]");
                    LOG64_NEW_LINE;
#endif
                    receive_callback->server_time(server_time);
                  }
                  else
                  {
                    receive_callback->received(tmp_buf, 0, size);
                  }

                  // schedule next buf and process teh same byte again
                  start_index = 0;

                }
                else
                {
                  tmp_buf[i - start_index] =  GPRS_DATA[i];
                  i++;
                }
              }

              GPRS_IS_RECV_DATA = false;
              GPRS_RECV_DATA_SIZE = 0;

              GPRS_EXECUTE_COMMAND(false);
            }
            else
            {
              // AT response in string format
              // check if this is what we expect
#if defined (LOG64_ENABLED)
              LOG64_SET(F("GPRS: RESPONSE["));
              LOG64_SET(GPRS_DATA);
              LOG64_SET(F("]"));
              //          LOG64_SET(F("{"));
              //          for (uint8_t k = 0; (k < 255) && (GPRS_DATA[k] != 0); k++)
              //          {
              //            LOG64_SET((uint8_t)GPRS_DATA[k]);
              //          }
              //          LOG64_SET(F("}"));
              LOG64_NEW_LINE;
#endif

              // check if any response is accepted
              const char * f_p = NULL;
              if (strcmp(GPRS_RESPONSE_ANY, GPRS_COMMAND[GPRS_COMMAND_I][GPRS_COMMAND_J][0]) == 0)
              {
                f_p = GPRS_RESPONSE_ANY;
              }
              else
              {
                f_p = strstr(GPRS_DATA, GPRS_COMMAND[GPRS_COMMAND_I][GPRS_COMMAND_J][0]);
              }

              // check for second option if first do not match - e,g, register local and roaming
              if (((f_p == NULL)) && (GPRS_DATA, GPRS_COMMAND[GPRS_COMMAND_I][GPRS_COMMAND_J][1] != NULL))
              {
                // and again check if any response is accepted
                if (strcmp(GPRS_RESPONSE_ANY, GPRS_COMMAND[GPRS_COMMAND_I][GPRS_COMMAND_J][1]) == 0)
                {
                  f_p = GPRS_RESPONSE_ANY;
                }
                else
                {
                  f_p = strstr(GPRS_DATA, GPRS_COMMAND[GPRS_COMMAND_I][GPRS_COMMAND_J][1]);
                }
              }

              if (f_p == NULL)
              {
#if defined (LOG64_ENABLED)
                //            LOG64_SET(F("GPRS: LAST["));
                //            LOG64_SET(GPRS_LAST_COMMAND);
                //            LOG64_SET(strlen(GPRS_LAST_COMMAND));
                //            LOG64_SET(F("]["));
                //            LOG64_SET(GPRS_DATA);
                //            LOG64_SET(strlen(GPRS_DATA));
                //            LOG64_SET(F("]"));
                //            LOG64_NEW_LINE;
#endif
                /// skip empty
                if (GPRS_IS_EMPTY(GPRS_DATA))
                {
#if defined (LOG64_ENABLED)
                  LOG64_SET(F("GPRS: RESPONSE IS EMPTY - SKIP."));
                  LOG64_NEW_LINE;
#endif
                }
                //check if ECHO
                else if (strcmp(GPRS_DATA, GPRS_LAST_COMMAND) == 0)
                {
#if defined (LOG64_ENABLED)
                  LOG64_SET(F("GPRS: RESPONSE IS ECHO SKIP."));
                  LOG64_NEW_LINE;
#endif
                }
                //check for noise
                else if (GPRS_IS_NOISE(GPRS_DATA, GPRS_SKIP_RESPONSE))
                {
#if defined (LOG64_ENABLED)
                  LOG64_SET(F("GPRS: RESPONSE IS NOISE - SKIP."));
                  LOG64_NEW_LINE;
#endif
                }
                else
                {

                  // something not expected received

                  // try if allowed
                  if (GPRS_OPERATION_RETRY > 0)
                  {
#if defined (LOG64_ENABLED)
                    LOG64_SET(F("GPRS: RESPONSE NOT EXPECTED - RETRY LEFT["));
                    LOG64_SET(GPRS_OPERATION_RETRY);
                    LOG64_SET(F("]"));
                    LOG64_NEW_LINE;
#endif
                    GPRS_COMMAND_J = 0;
                    GPRS_OPERATION_RETRY--;
                    // read all from serial
                    for (; GPRS_SERIAL.available();)
                    {
                      GPRS_SERIAL.read();
                    }

                    GPRS_EXECUTE_COMMAND(true);
                  }
                  else
                  {
                    // RETRY EXHAUSTED
                    //reset and start from the beggining
#if defined (LOG64_ENABLED)
                    LOG64_SET(F("GPRS: RESPONSE NOT EXPECTED - RESET"));
                    LOG64_NEW_LINE;
#endif


                    if (GPRS_COMMAND_I <= GPRS_SIGNAL_INDEX)
                    {
                      GPRS_SET_STATUS(GPRS_STATUS_NA, 0 );
                    }

                    if (GPRS_COMMAND_I == 0)
                    {
#if defined (LOG64_ENABLED)
                      LOG64_SET(F("GPRS: RESET ON SOFT RESET - DOING HARDWARE RESET"));
                      LOG64_NEW_LINE;
#endif
                      GPRS_HARD_RESET(false);

                      // skip everything else and return
                      return;
                    }

                    GPRS_COMMAND_I = 0;
                    GPRS_COMMAND_J = 0;
                    GPRS_IS_RECV_DATA = false;

                    // check if too many soft reset and need of hard reset
                    if (GPRS_SOFT_RESET_COUNT > GPRS_SOFT_RESET_MAX)
                    {
                      GPRS_HARD_RESET(true);

                      // skip everything else and return
                      return;
                    }

                    // read all from serial
                    for (; GPRS_SERIAL.available();)
                    {
                      GPRS_SERIAL.read();
                    }



                    GPRS_EXECUTE_COMMAND(false);
                  }
                }
              }
              else
              {
#if defined (LOG64_ENABLED)
                //            LOG64_SET(F("GPRS: RESPONSE CORRECT"));
                //            LOG64_NEW_LINE;
#endif

                GPRS_EXECUTE_COMMAND(false);
              }
            }

            GPRS_SIZE = 0;


          }
        }
      }
    }


  private:


    char GPRS_APN[253];
    char GPRS_USER[63];
    char GPRS_PASS[63];

    char SERVER_HOST[253];
    uint32_t SERVER_PORT;

    IoTGatewayGprsCallback * receive_callback;

    HardwareSerial GPRS_SERIAL;

    char GPRS_LAST_COMMAND[GPRS_MAX_BUF_SIZE + 1];

    uint8_t GPRS_QUEUE_BUF[GPRS_MAX_BUF_SIZE];
    uint16_t GPRS_QUEUE_BUF_SIZE;

    uint8_t GPRS_BUF_IN_NETWORK[GPRS_MAX_BUF_SIZE];
    uint16_t GPRS_BUF_IN_NETWORK_SIZE;

    char GPRS_SERVER_IP[GPRS_MAX_BUF_SIZE];
    uint16_t GPRS_DNS_AT_INDEX;
    const char * GPRS_DNS_AT;
    uint16_t GPRS_DNS_AT_SIZE;

    uint16_t GPRS_RECEIVE_AT_INDEX;
    const char * GPRS_RECEIVE_AT;
    uint16_t GPRS_RECEIVE_AT_SIZE;

    uint16_t GPRS_SIGNAL_INDEX;
    const char * GPRS_SIGNAL_AT;
    uint16_t GPRS_SIGNAL_SIZE;

    uint16_t GPRS_TCPSEND_RESPONSE_INDEX;
    const char * GPRS_TCPSEND_RESPONSE;
    uint16_t GPRS_TCPSEND_RESPONSE_SIZE;

    uint16_t GPRS_COMMAND_I;
    uint16_t GPRS_COMMAND_J;

    char GPRS_DATA[GPRS_MAX_RECV_BUF_SIZE];
    uint16_t  GPRS_SIZE;
    bool GPRS_IS_SERVER_IP;
    bool GPRS_IS_RECV_DATA;
    uint16_t GPRS_RECV_DATA_SIZE;
    uint16_t GPRS_RECV_TEMP_SIZE;
    uint32_t GPRS_RECV_START_TIME;
    bool GPRS_SIGNAL_DATA;


    uint32_t GPRS_OPERATION_START_TIME;
    uint32_t GPRS_OPERATION_RETRY;

    const char * GPRS_STATUS;
    bool GPRS_FIRST_HARDWARE_RESET;

    uint8_t GPRS_SOFT_RESET_COUNT;

    bool GPRS_DO;

    uint8_t GPRS_INIT_INDEX;
    uint32_t GPRS_INIT_TIME;

    // max is 100%
    char GPRS_CSQ_STRENGTH[5];
    uint8_t GPRS_CSQ_STRENGTH_PERCENT;

    uint32_t GPRS_PIN_RX;
    uint32_t GPRS_PIN_TX;
    uint32_t GPRS_PIN_RESET;
};


#pragma pop_macro("LOG64_ENABLED")
