/*
  K32_mqtt.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_mqtt_h
#define K32_mqtt_h

class K32_mqtt;      // prevent cicular include error
#include "K32.h"


class K32_mqtt {
  public:
    K32_mqtt(mqttconf conf, K32* engine);

//     const char* id_path();
//     const char* chan_path();
//     OSCMessage status();
//     void send(OSCMessage msg);

  private:
    SemaphoreHandle_t lock;
//     static void server( void * parameter );
//     static void beacon( void * parameter );
//     static void beat( void * parameter );

//     WiFiUDP* udp;         // must be protected with lock 
//     WiFiUDP* sendSock;
//     IPAddress linkedIP;

    mqttconf conf;
    K32* engine;
};

#endif
