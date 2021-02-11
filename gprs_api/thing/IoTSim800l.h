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
//GPRS SIM800L

// CONNECT/PIN
// GND -> GND
// VCC -> 3.3v - 4.2V
// TXD -> 21
// RXD -> 13
// GND -> GND

// fomrat
// {
// {COMMAND, NULL},
// {COMMAND1, NULL},
// {RESULT1, RESULT1 ALTERNATIVE}
// {RESULT2, RESULT2 ALTERNATIVE}
// {RESULT3, RESULT3 ALTERNATIVE}
// }
// NULL means empty
//
// GPRS_RECEIVE_AT_INDEX need to be set to pouint the RESULT that will indicate that data is received from the TCP
// GPRS_DNS_AT_INDEX need to be set to pouint the RESULT that will indicate the DNS response
// GPRS_TCPSEND_RESPONSE_INDEX need to be set - as no \r or \n will be returned from the modem in this case
// GPRS_SIGNAL_INDEX need to be set to pouint the GPS SIGNAL QUERY
const char * GPRS_COMMAND[GPRS_STATE_LEN][4][2] =
{
  {
    { "AT+CFUN=1,1", NULL}, // 0 soft reset
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "ATE0", NULL}, // 1 ECHO OFF
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
   {
    { "AT+CFUN=1", NULL}, // 2 all services
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
   {
    { "AT+CNMI=0", NULL}, // 3 stop SMS notify
    { GPRS_RESPONSE_ANY, NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CPIN?", NULL}, // 4 query SIM card available
    { "+CPIN: READY", NULL},
    { "OK", NULL},
    { NULL, NULL}
  },
  {
    { "AT+CREG?", NULL}, // 5 query registration
    { "+CREG: 0,1", "+CREG: 0,5"},
    { "OK", NULL},
    { NULL, NULL}
  },
  {
    { "AT+CSQ", NULL}, // 6 query signal custom handling make sure GPRS_SIGNAL_INDEX is set to this index
    { "+CSQ:", NULL},
    { "OK", NULL},
    { NULL, NULL}
  },
  {
    { "AT+CIPSHUT", NULL}, // 7 deactivate the bearer context
    { GPRS_RESPONSE_ANY, NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CSTT=\"@APN@\",\"@USER@\",\"@PASS@\"", NULL}, // 8 APN, USER, PASS
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CGATT=1", NULL}, // 9 set the GPRS attach
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CGATT?", NULL}, // 10 query the GPRS attach status
    { "+CGATT: 1", NULL},
    { "OK", NULL},
    { NULL, NULL}
  },
  {
    { "AT+CIICR", NULL}, // 11 activate the connection
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CIFSR", NULL}, // 12 query the PPP connection status
    { GPRS_RESPONSE_ANY, NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CIPQSEND=0", NULL}, // 13 Set normal mode
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CIPSPRT=1", NULL}, // 14 set ">" and SEND OK
    { "OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },

  {
    { "AT+CIPSTART=\"TCP\",\"@SERVER_HOST@\",\"@SERVER_PORT@\"", NULL}, // 15 setup tcp connection
    { "OK", NULL},
    { "CONNECT OK", NULL},
    { NULL, NULL}
  },
  {
    { "AT+CIPSEND=@DATA_LEN@", NULL}, // 16 tcp send (1, size) custom handling make sure GPRS_TCPSEND_RESPONSE_INDEX is set to this index
    { ">", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "@DATA@", NULL}, // 17 data send special handling when you send the actual data you receive first " " next SEND OK next 0x0A and next the server response -- custom handling - make sure you have set GPRS_DATA_AT_INDEX to point this index
    { "SEND OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { NULL, NULL}, // 18 data receive - custom handling - make sure you have set GPRS_RECEIVE_AT_INDEX to point this index
    { NULL, NULL}, 
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CIPCLOSE=0", NULL}, // 19 tcp close
    { "CLOSE OK", NULL},
    { NULL, NULL},
    { NULL, NULL}
  },
  {
    { "AT+CGATT=0", NULL}, // 20 detach PPP
    { GPRS_RESPONSE_ANY, NULL}, // +PDP: DEACT ( any to handle ocasanal not allowed)
    { GPRS_RESPONSE_ANY, NULL}, // OK ( any to handle ocasanal not allowed)
    { NULL, NULL}
  }
};

// max timeout for operation in msec
uint32_t GPRS_TIMEOUT[GPRS_STATE_LEN] =
{
  400000, // 0 soft restart
  20000,  // 1 echo off
  20000,  // 2 all services
  20000,  // 3 stop sms notify
  60000,  // 4 query SIM card available
  210000, // 5 query registration
  30000,  // 6 query signal
  30000,  // 7 deactivate the bearer context
  15000,  // 8 APN, USER, PASS
  30000,  // 9 set the GPRS attach
  30000,  // 10 query the GPRS attach status
  60000,  // 11 activate the PPP connection
  30000,  // 12 query the PPP connection status
  15000,  // 13 Set normal mode
  15000,  // 14 set ">" and SEND OK
  30000,  // 15 setup tcp connection
  30000,  // 16 tcp send
  120000, // 17 data send
  120000, // 18 data receive
  30000,  // 19 tcp close
  30000   // 20 detach PPP
};

// wait after execute the command - this will eat from the timeout - e.g. make sure the timeot is properly calculated
uint32_t GPRS_TIMEOUT_BEFORE_RESULT[GPRS_STATE_LEN] =
{
  100000,  // 0 soft restart
  500,   // 1 echo off
  500,   // 1 all services
  500,   // 1 stop sms noifyy
  500,  // 2 query SIM card available
  5000,   // 3 query registration
  5000,    // 4 query signal
  500,   // 5 deactivate the bearer context
  5000,   // 6 APN, USER, PASS
  5000,   // 7 set the GPRS attach
  5000,   // 8 query the GPRS attach status
  5000,   // 9 activate the PPP connection
  5000,    // 10 query the PPP connection status
  500,   // 11 Set normal mode
  500,    // 12 set ">" and SEND OK
  1000,   // 13 setup tcp connection
  0,    // 14 tcp send
  0,      // 15 data send
  0,      // 16 data receive
  500,    // 17 tcp close
  500     // 18 detach PPP
};


// how many times to retry a command before giving up and reset
uint32_t GPRS_COMMAND_RETRY[GPRS_STATE_LEN] =
{
  0,    // 0 soft restart
  1,    // 1 echo off
  0,    // 1 all services
  0,    // 1 estop sms notify
  5,    // 2 query SIM card available
  40,   // 3 query registration
  5,    // 4 query signal
  0,    // 5 deactivate the bearer context
  2,    // 6 APN, USER, PASS
  5,    // 7 set the GPRS attach
  5,    // 8 query the GPRS attach status
  5,    // 9 activate the PPP connection
  5,    // 10 query the PPP connection status
  0,   // 11 Set normal mode
  0,    // 12 set ">" and SEND OK
  20,    // 13 setup tcp connection
  0,    // 14 tcp send
  0,    // 15 data send
  0,    // 16 data receive
  0,    // 17 tcp close
  0     // 18 detach PPP
};
