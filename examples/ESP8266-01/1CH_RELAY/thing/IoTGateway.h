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

#pragma push_macro("LOG64_ENABLED")
//#undef LOG64_ENABLED

#include "IoTGatewayWifi.h"

#define IoTGateway_MIN_RSSI  (-150)
#define IoTGateway_MAGIC 8606
#define IoTGateway_OPER_DATA 128
#define IoTGateway_SEQ_COUNTER_MIN 2
#define IoTGateway_HEARTBEAT_TIMEOUT 60000
#define IoTGateway_HEARTBEAT_MAX_MESSAGE_LEN 16
#define IoTGateway_OPER_HEARTBEAT_IOT 127
#define IoTGateway_WAITING_NA 0
#define IoTGateway_WAITING_SENT 1
#define IoTGateway_WAITING_CONFIRM 2

class IoTGatewayCallback
{
  public :
    virtual void received(uint8_t buf_[], uint8_t size_) = 0;
    virtual void data_sent_successfully() = 0;
};

class IoTGateway : public IoTGatewayWifiCallback
{
  public :

    IoTGateway(const char * ssid_,
               const char * pass_,
               uint64_t gateway_id_,
               const char * gateway_key_,
               IoTGatewayCallback * receive_callback_) :
      receive_callback(receive_callback_),
      gateway_id(gateway_id_),
      gateway_seq(IoTGateway_SEQ_COUNTER_MIN),
      heartbeat_last(IoTGateway_HEARTBEAT_TIMEOUT),
      waiting_type(IoTGateway_WAITING_NA),
      oper_magic_counter(0)
    {

      hex_to_key(gateway_key_, gateway_key);

      wifi = new IoTGatewayWifi(ssid_, pass_, this);
#if defined (LOG64_ENABLED)
      LOG64_SET(F("IoTG: INIT"));
      LOG64_NEW_LINE;
#endif
    }

    // call before using
    void init()
    {
      wifi->init();
      gen_heartbeat();
    }


  public:

    // set gateway id ( in case after start needs to be changed)
    void set_gateway_id(uint64_t gateway_id_ )
    {
      gateway_id = gateway_id_;
    }

    // set gateway key ( in case after start needs to be changed)
    void set_gateway_key(const char *  gateway_key_)
    {
      hex_to_key(gateway_key_, gateway_key);
    }

    // set ssid pass ( in case after start needs to be changed)
    void set_ssid_pass(const char * ssid_, const char * pass_)
    {
      wifi->set_ssid_pass(ssid_, pass_);
    }


  public:

    virtual void received(uint8_t buf_[], uint8_t start_index_, uint8_t size_)
    {
      // init message
      buf_[start_index_ + 1] = (gateway_seq++);
      if (gateway_seq > 126)
      {
        gateway_seq = IoTGateway_SEQ_COUNTER_MIN;
      }

      buf_[start_index_] = crc(&buf_[start_index_], ((uint16_t)(start_index_ + size_)) , start_index_ + 1);

      receive_callback->received(&buf_[start_index_], size_);
    }

    virtual void data_sent(uint16_t size_)
    {
      if (waiting_type == IoTGateway_WAITING_SENT)
      {  
         waiting_type = IoTGateway_WAITING_CONFIRM;
      }
    }
    
    virtual void server_time(uint32_t time_)
    {
      if (waiting_type == IoTGateway_WAITING_CONFIRM)
      {  
         waiting_type = IoTGateway_WAITING_NA;
         receive_callback->data_sent_successfully();
      }
    }

  private:

    uint8_t char_int(char input)
    {
      if (input >= '0' && input <= '9')
      {
        return input - '0';
      }
      else if ((input >= 'A') && (input <= 'F'))
      {
        return input - 'A' + 10;
      }
      else if ((input >= 'a') && (input <= 'f'))
      {
        return input - 'a' + 10;
      }
      return 0;
    }

    // This function assumes src to be a zero terminated sanitized string with
    // an even number of [0-9a-fA-F] characters, and target to be sufficiently large
    void hex_to_key(const char * src, uint8_t target[16])
    {
      for (int i = 0; i < 16; i++)
      {
        target[i] = (char_int(src[i << 1]) << 4) | char_int(src[(i << 1) + 1]);
      }
    }


    void gen_heartbeat()
    {

      uint8_t buf[IoTGateway_HEARTBEAT_MAX_MESSAGE_LEN];
      uint8_t size = 0;

      // gen packet
      uint16_t magic = 6;
      magic = magic << 8;
      magic += (oper_magic_counter++);

      buf[size++] = 11;

      buf[size++] = IoTGateway_OPER_HEARTBEAT_IOT;

      buf[size++] = ((uint8_t*)&magic)[0];
      buf[size++] = ((uint8_t*)&magic)[1];

      buf[size++] = ((uint8_t*)&gateway_id)[0];
      buf[size++] = ((uint8_t*)&gateway_id)[1];
      buf[size++] = ((uint8_t*)&gateway_id)[2];
      buf[size++] = ((uint8_t*)&gateway_id)[3];

      buf[size++] = 0;//((uint8_t*)&WIFI_LAST_PACKET_TIME)[0];
      buf[size++] = 0;//((uint8_t*)&WIFI_LAST_PACKET_TIME)[1];
      buf[size++] = 0;
      buf[size++] = 0;

      wifi->send(buf, size);

#if defined (LOG64_ENABLED)
      LOG64_SET("HEARTBEAT INFO ASYNC SENT");
      LOG64_NEW_LINE;
#endif

    }

    void gen_internet_message(uint8_t out_buf_[], uint8_t & out_size_, uint8_t in_buf_[], uint8_t in_size_)
    {
      uint32_t crc = crc4(in_buf_, in_size_);

      // using out_buf as a temp storage
      memcpy(out_buf_, &crc, 4);
      memcpy(&out_buf_[4], in_buf_, in_size_);
      uint8_t *enc;
      size_t enc_size;

      enc =  xxtea_encrypt(out_buf_, in_size_ + 4, gateway_key,  &enc_size);


      int16_t rssi_ = (IoTGateway_MIN_RSSI  * (100 - ((int16_t)wifi->signal_strength()))) / 100;

      uint16_t magic = 86;
      magic = magic << 8;
      magic += (gateway_seq++);
      if (gateway_seq > 126)
      {
        gateway_seq = IoTGateway_SEQ_COUNTER_MIN;
      }

      out_buf_[0] = 9 + enc_size;

      out_buf_[1] = IoTGateway_OPER_DATA;

      out_buf_[2] = ((uint8_t*)&magic)[0];
      out_buf_[3] = ((uint8_t*)&magic)[1];

      out_buf_[4] = ((uint8_t*)&rssi_)[0];
      out_buf_[5] = ((uint8_t*)&rssi_)[1];

      out_buf_[6] = ((uint8_t*)&gateway_id)[0];
      out_buf_[7] = ((uint8_t*)&gateway_id)[1];
      out_buf_[8] = ((uint8_t*)&gateway_id)[2];
      out_buf_[9] = ((uint8_t*)&gateway_id)[3];

      memcpy(&out_buf_[10], enc, enc_size);

      out_size_ = enc_size + 10;
      free(enc);
    }

  public:

    // return signal strength in %
    uint8_t signal_strength()
    {
      return wifi->signal_strength();
    }


  public :

    void send(uint8_t buf_[], uint8_t size_)
    {
      uint8_t buf_wifi[128];
      uint8_t size_wifi;

      gen_internet_message(buf_wifi, size_wifi, buf_, size_);

      wifi->send(buf_wifi, size_wifi);

      waiting_type = IoTGateway_WAITING_SENT;
    }

    // please call in the main loop to be able to dispatch data and menage logic
    void work()
    {
      if (((uint32_t)(((uint32_t)millis()) - heartbeat_last)) >= IoTGateway_HEARTBEAT_TIMEOUT)
      {
        heartbeat_last = millis();
        gen_heartbeat();
      }
      
      wifi->work();
    }

  private :

    // calculate 4 byte checksum
    uint32_t crc4(uint8_t buf_[],
                  uint8_t size_)
    {
      uint32_t res = 0;
      for (uint8_t i = 0; i < size_; i++)
      {
        uint8_t c = buf_[i];
        res = (res << 1) ^ c;
        res = res & 0xFFFFFFFF;
      }
      return res;
    };


    // calculate checksum
    uint8_t crc(uint8_t buf_[],
                uint16_t end_index_,
                uint16_t start_index)
    {
      uint32_t res = IoTGateway_MAGIC;  // add magic
      for (uint16_t i = start_index; i < end_index_; i++)
      {
        uint8_t c = buf_[i];
        res = (res << 1) ^ c;
        res = res & 0xFFFFFFFF;
      }
      return (uint8_t)(res & 0xFF);
    };

  private :

    IoTGatewayCallback * receive_callback;

    uint8_t waiting_type;
    
    uint8_t gateway_seq;

    uint32_t heartbeat_last;
    uint8_t oper_magic_counter;

    uint8_t __attribute__ ((aligned (4))) gateway_key[16];
    uint64_t gateway_id;

    IoTGatewayWifi *  wifi;


  private :

    /**********************************************************\
      |                                                          |
      | XXTEA encryption algorithm library for C.                |
      |                                                          |
      | Encryption Algorithm Authors:                            |
      |      David J. Wheeler                                    |
      |      Roger M. Needham                                    |
      |                                                          |
      | Code Authors: Chen fei <cf850118@163.com>                |
      |               Ma Bingyao <mabingyao@gmail.com>           |
      | LastModified: Feb 7, 2016                                |
      |                                                          |
      | With changes                                             |
      |                                                          |
      | License (MIT)                                            |
      |                                                          |
      \**********************************************************/

#define MX (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z))
#define DELTA 0x9e3779b9

    uint32_t * xxtea_to_uint_array_size(const uint8_t * data, size_t len, size_t * out_len) {
      uint32_t *out;

      ++len;
      size_t n = (((len & 3) == 0) ? (len >> 2) : ((len >> 2) + 1));
      if (n < 2) n = 2;
      --len;

      out = (uint32_t *)malloc(n << 2);
      if (!out) return NULL;
      *out_len = n;

      memcpy(&((uint8_t *)out)[1], data, len);
      ((uint8_t *)out)[0] = (uint8_t)len;

      return out;
    }


    uint8_t * xxtea_to_ubyte_array(const uint32_t * data, size_t len, size_t * out_len) {
      uint8_t *out;
      size_t n;

      n = len << 2;
      out = (uint8_t *)malloc(n);

      memcpy(out, data, n);

      *out_len = n;

      return out;
    }

    uint32_t * xxtea_uint_encrypt(uint32_t * data, size_t len, uint32_t * key)
    {
      uint32_t n = (uint32_t)len - 1;
      uint32_t z = data[n], y, p, q = 6 + 52 / (n + 1), sum = 0, e;

      if (n < 1) return data;

      while (0 < q--)
      {
        sum += DELTA;
        e = sum >> 2 & 3;

        for (p = 0; p < n; p++)
        {
          y = data[p + 1];
          z = data[p] += MX;
        }

        y = data[0];
        z = data[n] += MX;
      }

      return data;
    }


    uint8_t * xxtea_encrypt(const uint8_t * data, size_t len, const uint8_t * key, size_t * out_len)
    {
      uint8_t *out;
      uint32_t *data_array;
      size_t data_len;

      if (!len) return NULL;

      data_array = xxtea_to_uint_array_size(data, len, &data_len);

      if (!data_array) return NULL;

      out = xxtea_to_ubyte_array(xxtea_uint_encrypt(data_array, data_len, (uint32_t *)key), data_len, out_len);

      free(data_array);

      return out;
    }
};

#pragma pop_macro("LOG64_ENABLED")
