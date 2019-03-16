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

//class IoTThing
//{
//  public:
//    // constructor
//    IoTThing(const char * ssid_,
//             const char * pass_,
//             uint64_t id_,
//             uint8_t key_[16],
//            uint64_t gateway_id_,
//             const char * gateway_key_,
//             void (* receive_callback_)(uint8_t buf_[], uint8_t size, int16_t rssi) = NULL
//            )
//
//    // call before using
//    void init();
//
//    // set id ( in case after start needs to be changed)
//    void set_id(uint64_t id_ );
//
//    // set key ( in case after start needs to be changed)
//    void set_key(uint8_t key_[16]);
//
//    // set gateway id ( in case after start needs to be changed)
//    void set_gateway_id(uint64_t gateway_id_ );
//
//    // set gateway key ( in case after start needs to be changed)
//    void set_gateway_key(uint8_t key_[16])
//
//    // set ssid and pass ( in case after start needs to be changed)
//    void set_ssid_pass(const char * ssid_, const char * pass_)
//
//    // register to receive callback after the message has been acknowledged from the gateway
//    void register_ack(void (* ack_callback_)(int16_t rssi_));
//
//    // register to receive callback after the message has failed/ in msec
//    void register_timeout(void (* timeout_callback_)(uint16_t timeout_));
//
//    // return signal strength in %
//    uint8_t signal_strength();
//
//    // return total messages sent
//    uint32_t total_messages();
//
//    //return total acknowledged messages send
//    uint32_t ack_messages();
//
//    //return received messages count
//    uint32_t recv_messages();
//
//    //return total retransmit messages send
//    uint32_t retransmit_messages()
//
//    //add discrete data
//    //use this if you want for example to store decimal(floating-point) temperature between -20 and +60 in one byte
//    // pos is the position from which the data will be written
//    // e.g. : add_discrete(buf, 0,  -20.0, 60.0, 23.65, 1);
//    //you need to ensure that you at least have container_size bytes available in the buffer
//    //1 <= conainer_size <= 4
//    //on return pos will have the value of first free byte(next postion after the data)
//    static void add_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, double value_, uint8_t container_size_);
//
//    //read discrete value
//    static double get_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, uint8_t container_size_);
//
//    //add unsigned integer data up to 62bit ( 0 - 4611686018427387904) - the container size will be adjusted automatically depending on the value
//    // pos is the position from which the data will be written
//    //you need to ensure that you have at least 8 bytes available in the buffer - as this is the maximum bytes that the number can occupy in worst case scenario
//    //on return pos will have the value of first free byte(next postion after the data)
//    static void add_uint(uint8_t buf_[], uint8_t &pos_, uint64_t value_);
//
//    // get unsigned integer value
//   static uint64_t get_uint(uint8_t buf_[], uint8_t &pos_)
//
//    // return false if a error is logged
//    // max message size in bytes is 31
//    bool send(uint8_t buf_[], uint8_t size_);
//
//    // return true - if all previous tasks has been completed and ready to accept new data
//    bool is_ready();
//
//    // call if you want to stop retry's to send the message
//     void cancel();
//
//    // return true - if last message sending has hit timeout and failed
//    bool timeout_hit();
//
//    // please call in the main loop to be able to dispatch data and menage logic
//    void work();
//};


#pragma push_macro("LOG64_ENABLED")
//#undef LOG64_ENABLED

#include "IoTGateway.h"

#define IoTThing_MIN_RSSI  (-150)
#define IoTThing_MAX_MESSAGE_LEN 31
#define IoTThing_MESSAGE_TIMEOUT 60000
#define IoTThing_MAGIC 8606
#define IoTThing_OPEN_GATEWAY_ID 10
#define IoTThing_WAIT_NA 0
#define IoTThing_WAIT_ACK 1
#define IoTThing_WAIT_TIMEOUT 2

class IoTThing : public IoTGatewayCallback
{
  public :

    IoTThing(const char * ssid_,
             const char * pass_,
             uint64_t id_,
             uint8_t key_[16],
             uint64_t gateway_id_,
             const char * gateway_key_,
             void (* receive_callback_)(uint8_t buf_[], uint8_t size, int16_t rssi) = NULL ) :
      id(id_),
      ack_callback(NULL),
      timeout_callback(NULL),
      wait_ack(IoTThing_WAIT_NA),
      receive_callback(receive_callback_),
      gateway_id(gateway_id_)
    {
      memcpy(key,  key_, 16);
      gateway = new IoTGateway(ssid_, pass_, gateway_id_, gateway_key_,  this);
    }

  public:
    // call before using
    void init()
    {
      gateway->init();

      total = 0;
      total_ack = 0;
      total_recv = 0;
      total_retransmit = 0;

      seq = random(126) + 1; // as * -1 is used for ack
      q = NULL;
      buf_size = 0;

#if defined (LOG64_ENABLED)
      LOG64_SET(F("IoTT: INIT"));
      LOG64_NEW_LINE;
#endif
    }

    // set id ( in case after start needs to be changed)
    void set_id(uint64_t id_ )
    {
      id = id_;
    }

    // set key ( in case after start needs to be changed)
    void set_key(uint8_t key_[16])
    {
      memcpy(key,  key_, 16);
    }

    // set gateway id ( in case after start needs to be changed)
    void set_gateway_id(uint64_t gateway_id_ )
    {
      gateway_id = gateway_id_;
      gateway->set_gateway_id(gateway_id_);
    }

    // set gateway key ( in case after start needs to be changed)
    void set_gateway_key(const char * gateway_key_)
    {
      gateway->set_gateway_key(gateway_key_);
    }

    // set ssid ( in case after start needs to be changed)
    void set_ssid_pass(const char * ssid_, const char * pass_)
    {
      gateway->set_ssid_pass(ssid_, pass_);
    }

    // register for to receive callback after the message has been acknowledged from the gateway
    void register_ack(void (* ack_callback_)(int16_t rssi))
    {
      ack_callback = ack_callback_;
    }

    // register to receive callback after the message has failed/ in msec
    void register_timeout(void (* timeout_callback_)(uint16_t timeout))
    {
      timeout_callback = timeout_callback_;
    }

  public:
    // return signal strength in %
    uint8_t signal_strength()
    {
      return gateway->signal_strength();
    }


    // return total messages sent
    uint32_t total_messages()
    {
      return total;
    }

    //return total acknowledged messages send
    uint32_t ack_messages()
    {
      return total_ack;
    }

    //return received messages count for the gateway
    uint32_t recv_messages()
    {
      return total_recv;
    }

    //return total acknowledged messages send
    uint32_t retransmit_messages()
    {
      return total_retransmit;
    }

  public:
    //add discrete data
    //use this if you want for example to store decimal(floating-point) temperature between -20 and +60 in one byte
    // pos is the position from which the data will be written
    // e.g. : add_discrete(buf, 0,  -20.0, 60.0, 23.65, 1);
    //you need to ensure that you at least have container_size bytes available in the buffer
    //1 <= conainer_size <= 4
    //on return pos will have the value of first free byte(next postion after the data)
    static void add_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, double value_, uint8_t container_size_)
    {
      double dev_;
      switch (container_size_)
      {
        case 1 : {
            dev_ = (max_ - min_) / 255.0d;
          } ; break;
        case 2 : {
            dev_ = (max_ - min_) / 65535.0d;
          } ; break;
        case 3 : {
            dev_ = (max_ - min_) / 16777215.0d;
          } ; break;
        case 4 : {
            dev_ = (max_ - min_) / 4294967295.0d;
          } ; break;
        default : dev_ = (max_ - min_) / 255.0d;
      };

      uint32_t  dis_ = (uint32_t)round((value_ - min_) / dev_);
      memcpy((void *)buf_[pos_], (void *) &dis_, container_size_);
      pos_ += container_size_;
    }

    static double get_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, uint8_t container_size_)
    {
      double dev_;
      switch (container_size_)
      {
        case 1 : {
            dev_ = (max_ - min_) / 255.0d;
          } ; break;
        case 2 : {
            dev_ = (max_ - min_) / 65535.0d;
          } ; break;
        case 3 : {
            dev_ = (max_ - min_) / 16777215.0d;
          } ; break;
        case 4 : {
            dev_ = (max_ - min_) / 4294967295.0d;
          } ; break;
        default : dev_ = (max_ - min_) / 255.0d;
      };

      uint32_t c = 0;
      memcpy((void *) &c, (void *)buf_[pos_], container_size_);
      pos_ += container_size_;
      return (min_ + (((double)c)  * dev_));
    }

    //add unsigned integer data up to 62bit ( 0 - 4611686018427387904) - the container will be adjusted automatically depending on the value
    //you need to ensure that you have at least 8 bytes available in the buffer - as this is the maximum bytes that the number can occupy in worst case scenario
    static void add_uint(uint8_t buf_[], uint8_t &pos_, uint64_t value_)
    {
      put_ots(value_, buf_, pos_);
    }

    static uint64_t get_uint(uint8_t buf_[], uint8_t &pos_)
    {
      return get_ots(buf_, pos_);
    }

  public:
    // return false if a error is logged
    // format byte(len), byte(crc), byte(seq), byte[4](receiver_id), byte[4](sender_id),byte[](encrypted_payload)
    // seq <=+127 - 0 is not valid sequence
    // payload max size 31
    bool send(uint8_t buf_[], uint8_t size_)
    {
      if (q != NULL)
      {
#if defined (LOG64_ENABLED)
        LOG64_SET(F("IoTT: NO SPACE FOR MSG."));
        LOG64_NEW_LINE;
#endif
        return false;
      }
      if  (size_ > (IoTThing_MAX_MESSAGE_LEN))
      {
#if defined (LOG64_ENABLED)
        LOG64_SET(F("IoTT: MSG TOO BIG["));
        LOG64_SET(size_);
        LOG64_SET(F("]"));
        LOG64_NEW_LINE;
#endif

        return false;
      }

      uint8_t *enc;
      size_t enc_size;

      enc = (uint8_t *) xxtea_encrypt(buf_, size_, key,  &enc_size);

      uint8_t raw[IoTThing_MAX_MESSAGE_LEN + 1 + 18];
      uint8_t pos_;
      //raw[0] = CRC
      raw[1] = seq;
      pos_ = 2;

      put_ots(gateway_id, raw, pos_ );
      put_ots(id, raw, pos_ );
      memcpy(&raw[pos_], enc , enc_size);
      free(enc);
      pos_ += enc_size;
      raw[0] = crc(raw, pos_, 1);

      memcpy(buf, raw , pos_);
      buf_size = pos_;
      q = buf;
      wait_ack = IoTThing_WAIT_ACK;
      wait_ack_start = millis();

      return true;
    }

    // return true - if all previouse tasks has been completed and ready to accept new data
    bool is_ready()
    {
      return (q == NULL) && ((wait_ack == IoTThing_WAIT_NA) || (wait_ack == IoTThing_WAIT_TIMEOUT));
    }

    // return true - if last message sending has hit timeout and failed
    bool timeout_hit()
    {
      return (wait_ack == IoTThing_WAIT_TIMEOUT);
    }

    // call if you want to stop retry's to send the message
    void cancel()
    {
      if (wait_ack == IoTThing_WAIT_ACK)
      {
        wait_ack = IoTThing_WAIT_TIMEOUT;
#if defined (LOG64_ENABLED)
        LOG64_SET(F("IoTT: MESSAGE CANCELED"));
        LOG64_NEW_LINE;
#endif
      }
    }

  public :

    virtual void received(uint8_t buf_[], uint8_t size_)
    {
      total_recv++;
      if (receive_callback != NULL)
      {
        int16_t rssi_ = (IoTGateway_MIN_RSSI  * (100 - ((int16_t)gateway->signal_strength()))) / 100;
        //crc
        //seq
        uint8_t sz = 2;
        get_ots(buf_, sz); // target
        get_ots(buf_, sz); // source
        size_t out_size = 0;
        uint8_t * dec = xxtea_decrypt(&buf_[sz], size_ - sz, key, & out_size);
        if ((dec != NULL) && (out_size > 0) && (dec[0] == crc(dec, (uint16_t)out_size, 1)))
        {
          receive_callback(&dec[1], (uint8_t)(out_size - 1), rssi_);
        }
        else
        {
#if defined (LOG64_ENABLED)
          LOG64_SET(F("IoTT: CRC CHECK FOR RECEIVED MSG FAILED"));
          LOG64_NEW_LINE;
#endif
        }
        if (dec != NULL)
        {
          free(dec);
        }
      }
    }

    virtual void data_sent_successfully()
    {
      wait_ack = IoTThing_WAIT_NA;
      // ack
      total_ack++;
      if (ack_callback != NULL)
      {

        int16_t rssi_ = (IoTGateway_MIN_RSSI  * (100 - ((int16_t)gateway->signal_strength()))) / 100;
        ack_callback(rssi_);
      }
    }

    // please call in the main loop to be able to dispatch data and menage logic
    void work()
    {
      gateway->work();

      if (wait_ack == IoTThing_WAIT_ACK)
      {
        if ((wait_ack == IoTThing_WAIT_ACK) && (((uint32_t)(((uint32_t)millis()) - wait_ack_start)) >= IoTThing_MESSAGE_TIMEOUT))
        {
          wait_ack == IoTThing_WAIT_TIMEOUT;
          if (timeout_callback != NULL)
          {
            timeout_callback(IoTThing_MESSAGE_TIMEOUT);
          }
          cancel();
        }
      }

      if (q != NULL)
      {
        gateway->send(buf, buf_size);
        total++;
        q = NULL;
        if ((++seq) > 126)
        {
          seq = 1; // as * -1 is used for ack
        }
        buf_size = 0;
      }
    };

  private:

    // calculate checksum
    uint8_t crc(uint8_t buf_[],
                uint16_t end_index_,
                uint16_t start_index)
    {
      uint32_t res = IoTThing_MAGIC;  // add magic
      for (uint16_t i = start_index; i < end_index_; i++)
      {
        uint8_t c = buf_[i];
        res = (res << 1) ^ c;
        res = res & 0xFFFFFFFF;
      }
      return (uint8_t)(res & 0xFF);
    }

  private:
    static uint64_t get_ots(uint8_t buf_[], uint8_t & pos_)
    {
      uint8_t start_ = pos_;
      switch (buf_[start_] & 0x3)
      {
        case 0:
          {
            pos_ += 1;
            return (uint64_t) ((buf_[start_] >> 2) & 0x3F);
          }
        case 1:
          {
            pos_ += 2;
            return (uint64_t)((((buf_[start_ + 1] << 8) | (buf_[start_])) >> 2) & 0x3FFF);
          }
        case 2:
          {
            pos_ += 4;
            return (uint64_t)((((((uint32_t)buf_[start_ + 3]) << 24) | (((uint32_t)buf_[start_ + 2]) << 16) | (((uint32_t)buf_[start_ + 1]) << 8) | ((uint32_t)buf_[start_])) >> 2) & 0x3FFFFFFFL);
          }
        case 3:
          {
            pos_ += 8;
            return  (((((uint64_t)buf_[start_ + 7]) << 56) | (((uint64_t)buf_[start_ + 6]) << 48) | (((uint64_t)buf_[start_ + 5]) << 40) | (((uint64_t)buf_[start_ + 4]) >> 32) | (((uint64_t)buf_[start_ + 3]) << 24)
                      | (((uint64_t)buf_[start_ + 2]) << 16) | (((uint64_t)buf_[start_ + 1]) << 8) | ((uint64_t)buf_[start_])) >> 2) & 0x3FFFFFFFFFFFFFFFLL;
          }
      }
    }

    static void put_ots(uint64_t v, uint8_t buf_[], uint8_t & pos_ )
    {
      v = (v << 2);
      uint8_t start_ = pos_;
      if (v < 0x40LL)
      {
        buf_[start_] = (uint8_t) (v & 0xFFLL);
        pos_ += 1;
      }
      else if (v < 0x4000LL)
      {
        v = v | 1;
        buf_[start_ + 1] = (uint8_t)((v >> 8) & 0xFFLL);
        buf_[start_] = (uint8_t) (v & 0xFFLL);
        pos_ += 2;
      }
      else if (v < 0x40000000LL)
      {
        v = v | 2;
        buf_[start_ + 3] = (uint8_t)((v >> 24) & 0xFFLL);
        buf_[start_ + 2] = (uint8_t)((v >> 16) & 0xFFLL);
        buf_[start_ + 1] = (uint8_t)((v >> 8) & 0xFFLL);
        buf_[start_] = (uint8_t) (v & 0xFFLL);
        pos_ += 4;
      }
      else if (v < 0x4000000000000000LL)
      {
        v = v | 3;
        buf_[start_ + 7] = (uint8_t)((v >> 56) & 0xFFLL);
        buf_[start_ + 6] = (uint8_t)((v >> 48) & 0xFFLL);
        buf_[start_ + 5] = (uint8_t)((v >> 40) & 0xFFLL);
        buf_[start_ + 4] = (uint8_t)((v >> 32) & 0xFFLL);
        buf_[start_ + 3] = (uint8_t)((v >> 24) & 0xFFLL);
        buf_[start_ + 2] = (uint8_t)((v >> 16) & 0xFFLL);
        buf_[start_ + 1] = (uint8_t)((v >> 8) & 0xFFLL);
        buf_[start_] = (uint8_t) (v & 0xFFLL);
        pos_ += 8;
      }
    }

  private :
    uint8_t buf[IoTThing_MAX_MESSAGE_LEN + 18 + 1];
    uint8_t buf_size;
    int8_t seq; // as * -1 is used for ack
    uint8_t * q; // used as flag

    uint32_t total;
    uint32_t total_ack;
    uint32_t total_recv;
    uint32_t total_retransmit;

    uint8_t resetPin;

    uint8_t __attribute__ ((aligned (4))) key[16];
    uint64_t id;

    void (* ack_callback)(int16_t rssi_);
    void (* timeout_callback)(uint16_t timeout_);

    uint64_t gateway_id;

    uint8_t wait_ack;
    uint32_t wait_ack_start;

    void (* receive_callback)(uint8_t buf_[], uint8_t size_, int16_t rssi_);

    IoTGateway *  gateway;

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


    uint32_t * xxtea_to_uint_array(const uint8_t * data, size_t len, size_t * out_len) {
      uint32_t *out;

      out = (uint32_t *)malloc(len);

      memcpy(out, data, len);

      *out_len = len >> 2;

      return out;
    }

    uint8_t * xxtea_to_ubyte_array_size(const uint32_t * data, size_t len, size_t * out_len) {
      uint8_t *out;
      size_t n;

      n = ((uint8_t *)data)[0];
      out = (uint8_t *)malloc(n);

      memcpy(out, &((uint8_t *)data)[1], n);

      *out_len = n;

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

    uint32_t * xxtea_uint_decrypt(uint32_t * data, size_t len, uint32_t * key)
    {
      uint32_t n = (uint32_t)len - 1;
      uint32_t z, y = data[0], p, q = 6 + 52 / (n + 1), sum = q * DELTA, e;

      if (n < 1) return data;

      while (sum != 0)
      {
        e = sum >> 2 & 3;

        for (p = n; p > 0; p--)
        {
          z = data[p - 1];
          y = data[p] -= MX;
        }

        z = data[n];
        y = data[0] -= MX;
        sum -= DELTA;
      }

      return data;
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


    uint8_t * xxtea_decrypt(const uint8_t * data, size_t len, const uint8_t * key, size_t * out_len)
    {
      uint8_t *out;
      uint32_t *data_array;
      size_t data_len;

      if (!len) return NULL;

      data_array = xxtea_to_uint_array(data, len, &data_len);
      if (!data_array) return NULL;


      out = xxtea_to_ubyte_array_size(xxtea_uint_decrypt(data_array, data_len, (uint32_t *)key), data_len, out_len);

      free(data_array);

      return out;
    }

};

#pragma pop_macro("LOG64_ENABLED")
