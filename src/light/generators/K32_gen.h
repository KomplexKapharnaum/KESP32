/*
  K32_gen.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_gen_h
#define K32_gen_h

#define LEDS_DATA_SLOTS  64

#include "../K32_ledstrip.h"

//
// NOTE: to be able to load an animation by name, it must be registered in K32_genbook
//

//
// BASE ANIM
//
class K32_gen {
  public:
    K32_gen(String name) {
      this->_name = name;
      this->newdata = xSemaphoreCreateMutex();
    }
    
    // retrieve name (set by anim class)
    String name () { return this->_name; };

    // init called when anim starts to play (can be overloaded by anim class)
    // waiting for initdata ensure that nothing is really played before first data are pushed
    // setting startTime can be usefull..
    virtual void init() { 
      this->waitData();
      this->refresh();
      this->startTime = millis(); 
    }

    // loop called by dedicated xtask
    // this is a prototype, mus be defined in specific anim class
    // return true/false to loop or not
    // can check or block waiting for new external data, or run on itself
    virtual bool loop ( K32_ledstrip* strip ) { 
      this->waitData();
      LOG("ANIM: newdata pushed !");
      return false; 
    };

    // change one element in data (do not trigger refresh !)
    virtual K32_gen* set(int k, int value) { 
      if (k < LEDS_DATA_SLOTS) this->data[k] = value; 
      return this;
    }

    // signal that data has been updated
    K32_gen* refresh() {
      xSemaphoreGive(this->newdata);
      return this;
    }

    // flush newdata flag: cancel programmed refresh
    K32_gen* flush() {
      xSemaphoreTake(this->newdata, (TickType_t) 1);  // lock it, if not already token
      return this;
    }

    // block until refresh is called
    K32_gen* waitData() {
      xSemaphoreTake(this->newdata, portMAX_DELAY);   // block until next refresh
      return this;
    }

    // set a new data and trigger refresh !
    K32_gen* push(int* frame, int size) {
      size = min(size, LEDS_DATA_SLOTS);
      for(int k=0; k<size; k++) this->set(k, frame[k]);
      return this->refresh();
    }

    // Helper to set and refresh various amount of data
    K32_gen* push() { return this->refresh(); }
    K32_gen* push(int d0) { return this->push(new int[1]{d0}, 1); }
    K32_gen* push(int d0, int d1) { return this->push(new int[2]{d0, d1}, 2); }
    K32_gen* push(int d0, int d1, int d2) { return this->push(new int[3]{d0, d1, d2}, 3); }
    K32_gen* push(int d0, int d1, int d2, int d3) { return this->push(new int[4]{d0, d1, d2, d3}, 4); }
    K32_gen* push(int d0, int d1, int d2, int d3, int d4) { return this->push(new int[5]{d0, d1, d2, d3, d4}, 5); }
    K32_gen* push(int d0, int d1, int d2, int d3, int d4, int d5) { return this->push(new int[6]{d0, d1, d2, d3, d4, d5}, 6); }
    K32_gen* push(int d0, int d1, int d2, int d3, int d4, int d5, int d6) { return this->push(new int[7]{d0, d1, d2, d3, d4, d5, d6}, 7); }
    K32_gen* push(int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7) { return this->push(new int[8]{d0, d1, d2, d3, d4, d5, d6, d7}, 8); }


  protected:

    void delay(int ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
    
    int data[LEDS_DATA_SLOTS];
    SemaphoreHandle_t newdata;  
    unsigned long startTime = 0;
    String _name = "?"; 
};


//
// ANIMATOR BOOK
//

#define LEDS_ANIMS_SLOTS  16

#include "K32_gen_basics.h"
#include "K32_gen_charge.h"
#include "K32_gen_dmx.h"

//
// NOTE: to be able to load an animation by name, it must be registered in K32_genbook
//

class K32_genbook {
  public:
    K32_genbook() {

      //
      // REGISTER AVAILABLE ANIMS HERE !
      //
      this->add( new K32_gen_test() );
      this->add( new K32_gen_color() );
      // this->add( new K32_gen_strobe() );
      // this->add( new K32_gen_hardstrobe() );
      // this->add( new K32_gen_chaser() );
      this->add( new K32_gen_discharge() );
      this->add( new K32_gen_charge() );
      this->add( new K32_gen_dmx() );

    }

    K32_gen* get( String name ) {
      for (int k=0; k<this->counter; k++)
        if (this->anims[k]->name() == name) {
          // LOGINL("LEDS: "); LOG(name);
          return this->anims[k];
        }
      LOGINL("ANIM: not found "); LOG(name);
      return new K32_gen("dummy");
    }


  private:
    K32_gen* anims[LEDS_ANIMS_SLOTS];
    int counter = 0;

    void add(K32_gen* anim) {
      if (this->counter >= LEDS_ANIMS_SLOTS) {
        LOG("ERROR: no more slot available to register new animation");
        return;
      }
      this->anims[ this->counter ] = anim;
      this->counter++;
      // LOGINL("ANIM: register "); LOG(anim->name());
    };

};


#endif