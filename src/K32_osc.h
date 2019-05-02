/*
  K32_osc.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_osc_h
#define K32_osc_h

class K32_osc;      // prevent cicular include error
#include "K32.h"

#include <WiFi.h>
#include <WiFiUdp.h>

#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

class K32_osc {
  public:
    K32_osc(oscconf conf, K32* engine);
    const char* id_path();
    const char* chan_path();

    OSCMessage status();

  private:
    SemaphoreHandle_t lock;
    static void server( void * parameter );
    static void beacon( void * parameter );
    static void beat( void * parameter );

    WiFiUDP* udp;         // must be protected with lock 
    IPAddress linkedIP;

    oscconf conf;
    K32* engine;
};

// OSCMessage overload
class K32_oscmsg : public OSCMessage {
  public:
    K32_oscmsg(K32_osc* env);

    bool dispatch(const char * pattern, void (*callback)(K32_osc* env, K32_oscmsg &), int = 0);
    bool route(const char * pattern, void (*callback)(K32_osc* env, K32_oscmsg &, int), int = 0);
    String getStr(int position);

    K32_osc* env;
};

#endif