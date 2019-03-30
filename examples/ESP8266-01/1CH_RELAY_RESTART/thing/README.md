The RESTARTER 

(the idea is to check internet connectivity and if not available to restart the modem)


Heartbeat is send every 4 minutes  and if not received acknowledge in 10 min ( from one of the two heart beats ) 
e.g. two heartbeats has been missed the relay is switched OFF for 10 sec and after that ON
The button, if available e.g. in case of Sonoff or TYWE2S is used to toggle from ON to OFF and vise verse
If the relay is off for 10 sec then it will be automatically switched ON


*FOR HOWTO UPLOAD/INSTALL - PLEASE READ : https://github.com/i4things/NodeAPI/blob/master/examples/ESP8266-01/1CH_RELAY/thing/README.md
