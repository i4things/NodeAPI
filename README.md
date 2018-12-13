# NodeAPI
Node/Device/Thing  Data Send/Receive API 

On the node/thing side i4things provide one simple straight forward class that can be used to deliver data to the authority/server.
```
class IoTThing
{
  public:
    // constructor
    IoTThing(uint8_t slaveSelectPin_,
             uint8_t interruptPin_,
             uint8_t resetPin_,

             uint8_t key_[16],
             uint64_t id_,
             void (* receive_callback_)(uint8_t buf_[], uint8_t size_, int16_t rssi_) = NULL,
             uint64_t gateway_id_ = 10); // default open gateway id

    // call before using
    void init();

    // set id ( in case after start needs to be changed)
    void set_id(uint64_t id_ );

    // set key ( in case after start needs to be changed)
    void set_key(uint8_t key_[16]);

    // set gateway id ( in case after start needs to be changed)
    void set_gateway_id(uint64_t gateway_id_ );

    // register to receive callback after the message has been acknowledged from the gateway
    void register_ack(void (* ack_callback_)(int16_t rssi_));

    // register to receive callback after the message has failed/ in msec
    void register_timeout(void (* timeout_callback_)(uint16_t timeout_));

    // return signal strength in %
    uint8_t signal_strength();

    // return total messages sent
    uint32_t total_messages();

    //return total acknowledged messages send
    uint32_t ack_messages();

    //return received messages count
    uint32_t recv_messages();

    //return total retransmit messages send
    uint32_t retransmit_messages()

    //add discrete data
    //use this if you want for example to store decimal(floating-point) temperature between -20 and +60 in one byte
    // pos is the position from which the data will be written
    // e.g. : add_discrete(buf, 0,  -20.0, 60.0, 23.65, 1);
    //you need to ensure that you at least have container_size bytes available in the buffer
    //1 <= conainer_size <= 4
    //on return pos will have the value of first free byte(next postion after the data)
    static void add_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, double value_, uint8_t container_size_);

    //read discrete value
    static double get_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, uint8_t container_size_);

    //add unsigned integer data up to 62bit ( 0 - 4611686018427387904) - the container size will be adjusted automatically depending on the value
    // pos is the position from which the data will be written
    //you need to ensure that you have at least 8 bytes available in the buffer - as this is the maximum bytes that the number can occupy in worst case scenario
    //on return pos will have the value of first free byte(next postion after the data)
    static void add_uint(uint8_t buf_[], uint8_t &pos_, uint64_t value_);

    // get unsigned integer value
   static uint64_t get_uint(uint8_t buf_[], uint8_t &pos_)

    // return false if a error is logged
    // max message size in bytes is 31
    bool send(uint8_t buf_[], uint8_t size_);

    // return true - if all previous tasks has been completed and ready to accept new data
    bool is_ready();

    // call if you want to stop retry's to send the message
     void cancel();

    // return true - if last message sending has hit timeout and failed
    bool timeout_hit();

    // please call in the main loop to be able to dispatch data and menage logic
    void work();
};

```

Example how to use the API:

#include "IoTThing.h"


// exmaple PINS for Feather 32u4
#define CS_PIN 8
#define RST_PIN 4
#define INT_PIN 7

#define DEVICE_ID 1000
uint8_t key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// you can also optionally pass:
// - function pointer/address for callback messages when received
// - gateway id - in case you want to use a private/closed gateway
//
// example for message received callback:
//
// called when packet received from node
// void received(uint8_t buf_[], uint8_t size_, int16_t rssi)
// {
//  //process message in buf_[] here with length = size_ 
// };
//
// IoTThing thing(CS_PIN, INT_PIN, RST_PIN, key, DEVICE_ID, received);
//

IoTThing thing(CS_PIN, INT_PIN, RST_PIN, key, DEVICE_ID);



// 2 minutes
#define MESSAGE_INTERVAL  120000
uint32_t MESSAGE_LAST_SEND;

void setup() {
  MESSAGE_LAST_SEND = millis() + MESSAGE_INTERVAL * 2;

  thing.init();
}



void loop() {
  // let IoT layer do it job
  thing.work();

  // try send message every 2 min
  if (((uint32_t)(((uint32_t)millis()) - MESSAGE_LAST_SEND))  >= MESSAGE_INTERVAL) {
    MESSAGE_LAST_SEND = millis();

    uint8_t msg[2];

    msg[0] = 22; // e.g. temperature 
    msg[1] = 78; // e.g. humidity

    // check if IoT layer is ready to accept new message
    if (thing.is_ready()) {
      thing.send(msg, 2);
    }
    else {
      // cancel previous work and send new message
      thing.cancel();
      thing.send(msg, 2);
    }
  }

}
IMPORTANT

For the simple open source node library you will also need to download and make available the RadioHead library from : https://www.airspayce.com/mikem/arduino/RadioHead/index.html

 

PRACTICALITY AND EFFICIENCY

In the API there are found very helpful static functions related to easy and efficient way of representing data in byte buffer(byte array) message :

static void add_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, double value_, uint8_t container_size_);
static double get_discrete(uint8_t buf_[], uint8_t &pos_, double min_, double max_, uint8_t container_size_);

static void add_uint(uint8_t buf_[], uint8_t &pos_, uint64_t value_);
static uint64_t get_uint(uint8_t buf_[], uint8_t &pos_);
Using the add/get discrete you can add to the buffer and read from the buffer discrete values - e.g. if you want to store temperate in the interval between -20C and +60C efficiently in only one byte but still having better resolution then 1/2 degree then you can use the discrete functions in the following fashion:

uint8_t buf[2];
uint8_t buf_pos = 0;

double temp1 = 23.5;
double temp2 = 18.1;

//insert into the buffer
IoTThing::add_discrete(buf, buf_pos,-20.0, 60.0, temp1, 1);
IoTThing::add_discrete(buf, buf_pos,-20.0, 60.0, temp2, 1);

buf_pos = 0;

double temp1_read_from_buffer = IoTThing::get_discrete(buf,buf_pos, -20.0, 60.0, 1);
double temp2_read_from_buffer = IoTThing::get_discrete(buf,buf_pos, -20.0, 60.0, 1);
Using the add/get uint you can add to the buffer and read from the buffer positive integer values between 0 and 4611686018427387904. The value will be stored in the minimum possible bytes e.g. if the value can fit in one byte it will occupy only one byte, if it can fit in 2 bytes it will occupy only two bytes and etc. up to 8 bytes. This way you can save space in the message, be efficient and optimize your traffic:

uint8_t buf[8]; // make sure we have space for maximum size
uint8_t buf_pos = 0;

//insert into the buffer
IoTThing::add_uint(buf, buf_pos, 11); // will occupy only 1 byte
IoTThing::add_uint(buf, buf_pos, 500); // will occupy only 2 bytes

buf_pos = 0;
int val1 = (int)IoTThing::get_uint(buf, buf_pos);
int val2 = (int)IoTThing::get_uint(buf, buf_pos);
