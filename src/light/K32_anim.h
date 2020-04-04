/*
  K32_anim.h
  Created by Thomas BOHL, february 2019.
  Released under GPL v3.0
*/
#ifndef K32_anim_h
#define K32_anim_h

#define LEDS_DATA_SLOTS  64

#include "K32_ledstrip.h"

//
// BASE ANIM
//
class K32_anim {
  public:
    K32_anim()
    {
      this->newData = xSemaphoreCreateBinary();
      this->dataInUse = xSemaphoreCreateBinary();
      xSemaphoreTake(this->newData, 1);
      xSemaphoreGive(this->dataInUse);

      this->stop_lock = xSemaphoreCreateBinary();
      this->wait_lock = xSemaphoreCreateBinary();
      xSemaphoreGive(this->stop_lock);
      xSemaphoreGive(this->wait_lock);

      this->_strip = NULL;

      memset(this->data, 0, sizeof this->data);
    }

    K32_anim* setup(K32_ledstrip* strip, int size = 0, int offset = 0) {
      this->_strip = strip;
      this->_size = (size) ? size : strip->size();
      this->_offset = offset;
      return this;
    }
    
    // ANIM CONTROLS
    //

    // get/set name
    String name () { 
      return this->_name; 
    }
    void name (String n) { 
      this->_name = n; 
    }

    int size() {
      return this->_size;
    }

    // Set FPS
    void fps(int f = -1) {
      if (f >= 0) _fps = f;
      else _fps = LEDS_ANIM8_FPS;
    }

    // loop get
    bool loop() { 
      return _loop;
    }

    // loop set
    K32_anim* loop(bool doLoop) { 
      _loop = doLoop;
      return this;
    }

    // is playing ?
    bool isPlaying() {
      xSemaphoreTake(this->stop_lock, portMAX_DELAY);
      bool r = (this->animateHandle != NULL);
      xSemaphoreGive(this->stop_lock);
      return r;
    }

    // start animate thread
    K32_anim* play(int timeout = 0) 
    {
      if (this->_strip == NULL) {
        LOG("ERROR: animation, you need to call setup before play !");
        return this;
      }
      this->_stopAt = (timeout>0) ? millis() + timeout : 0;
      if (this->isPlaying()) return this;
      this->stop();

      xSemaphoreTake(this->wait_lock, portMAX_DELAY);
      xSemaphoreTake(this->dataInUse, portMAX_DELAY);
      xTaskCreate( this->draw,           // function
                    "anim_task",            // task name
                    5000,                   // stack memory
                    (void*)this,            // args
                    3,                      // priority
                    &this->animateHandle ); // handler
      LOGF("ANIM: %s play \n", this->name() );
      
      return this;
    }

    // stop thread
    K32_anim* stop() {
      if (!this->isPlaying()) return this;
      this->_strip->lock();
      xSemaphoreTake(this->stop_lock, portMAX_DELAY);
      if (this->animateHandle != NULL) {
        vTaskDelete( this->animateHandle );
      }
      this->_strip->unlock();
      this->release();
      xSemaphoreGive(this->stop_lock);
      LOG("LIGHT: stop");
      return this;
    }

    void release() 
    {
      this->animateHandle = NULL;
      xSemaphoreTake(this->newData, 1);
      xSemaphoreGive(this->dataInUse);
      xSemaphoreGive(this->wait_lock);
      this->clear();
    }

    // wait until anim end (or timeout)
    bool wait(int timeout = 0) {
      TickType_t xTicksToWait = portMAX_DELAY;
      if (timeout > 0) xTicksToWait = pdMS_TO_TICKS(timeout);

      if ( xSemaphoreTake(this->wait_lock, xTicksToWait) == pdTRUE) {
        xSemaphoreGive(this->wait_lock);
        return true;
      }
      return false;
    }

    // ANIM LOGIC
    //

    // init called when anim starts to play (can be overloaded by anim class)
    // waiting for data ensure that nothing is really played before first data are set
    // setting startTime can be usefull..
    virtual void init() { 
      xSemaphoreTake(this->newData, portMAX_DELAY);   // wait for new data before starting
      xSemaphoreGive(this->newData);
      this->startTime = millis();
    }

    // generate frame from data, called by update
    // this is a prototype, must be defined in specific anim class
    virtual void frame (int data[LEDS_DATA_SLOTS]) {
      LOG("ANIM: nothing to do..");
    };
    

    // modul8 called by dedicated xtask
    // execute all registered modulators to change data (external data) and idata (internal data)
    virtual void modulate () {
      // this->refresh();
    };


    // ANIM DATA
    //

    // signal that data has been updated -> unblock waitData
    K32_anim* refresh() {
      xSemaphoreGive(this->newData);
      return this;
    }

    // change one element in data
    K32_anim* setdata(int k, int value) { 
      xSemaphoreTake(this->dataInUse, portMAX_DELAY);     // data can be modified only during anim waitData
      if (k < LEDS_DATA_SLOTS) this->data[k] = value; 
      xSemaphoreGive(this->dataInUse);
      return this;
    }

    // new data set 
    K32_anim* set(int* frame, int size) {
      size = min(size, LEDS_DATA_SLOTS);
      xSemaphoreTake(this->dataInUse, portMAX_DELAY);     // data can be modified only during anim waitData
      for(int k=0; k<size; k++) this->data[k] = frame[k]; 
      xSemaphoreGive(this->dataInUse);
      this->refresh();
      return this;
    }
    
    // new data set 
    K32_anim* set(uint8_t* frame, int size) {
      size = min(size, LEDS_DATA_SLOTS);
      xSemaphoreTake(this->dataInUse, portMAX_DELAY);     // data can be modified only during anim waitData
      for(int k=0; k<size; k++) this->data[k] = frame[k]; 
      xSemaphoreGive(this->dataInUse);
      this->refresh();
      return this;
    } 

    // Helper to set and refresh various amount of data
    K32_anim* set(int d0) { return this->set(new int[1]{d0}, 1); }
    K32_anim* set(int d0, int d1) { return this->set(new int[2]{d0, d1}, 2); }
    K32_anim* set(int d0, int d1, int d2) { return this->set(new int[3]{d0, d1, d2}, 3); }
    K32_anim* set(int d0, int d1, int d2, int d3) { return this->set(new int[4]{d0, d1, d2, d3}, 4); }
    K32_anim* set(int d0, int d1, int d2, int d3, int d4) { return this->set(new int[5]{d0, d1, d2, d3, d4}, 5); }
    K32_anim* set(int d0, int d1, int d2, int d3, int d4, int d5) { return this->set(new int[6]{d0, d1, d2, d3, d4, d5}, 6); }
    K32_anim* set(int d0, int d1, int d2, int d3, int d4, int d5, int d6) { return this->set(new int[7]{d0, d1, d2, d3, d4, d5, d6}, 7); }
    K32_anim* set(int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7) { return this->set(new int[8]{d0, d1, d2, d3, d4, d5, d6, d7}, 8); }



  // PROTECTED
  //
  protected:

    // draw pix
    void pixel(int pix, CRGBW color)  {
      if (pix < this->_size)
        this->_strip->pix( pix + this->_offset, color);
    }

    // draw multiple pix
    void pixel(int pixStart, int count, CRGBW color)  {
      this->_strip->pix( pixStart + this->_offset, count, color);
    }

    // draw all
    void all(CRGBW color) {
      this->_strip->pix( this->_offset, this->_size, color);
    }

    // clear
    void clear() {
      this->all({0,0,0,0});
    }
    
    unsigned long startTime = 0;


  // PRIVATE
  //
  private:

    // input data
    int data[LEDS_DATA_SLOTS];

    // THREAD: draw frame on new data
    static void draw( void * parameter ) 
    {
      K32_anim* that = (K32_anim*) parameter;
      TickType_t timeout;

      that->init();

      do 
      {
        xSemaphoreGive(that->dataInUse);                                          // allow set & setdata to modify data
        yield();                                                                          
        timeout = pdMS_TO_TICKS( 1000/that->_fps );                               // set timeout according to FPS
        
        if (xSemaphoreTake(that->newData, timeout) == pdTRUE)                     // Wait external DATA refresh or FPS timeout
          xSemaphoreGive(that->newData);

        xSemaphoreTake(that->dataInUse, portMAX_DELAY);                           // lock data to prevent new change until next loop
        
        if (that->_stopAt && millis() >= that->_stopAt) break;                    // check duration, set at play()

        that->modulate();           // data are locked, can not use set() or setdata(), if change applied must call refresh !

        if (xSemaphoreTake(that->newData, 0) == pdTRUE)                           // new data available
        {
          int datacopy[LEDS_DATA_SLOTS];
          memcpy(datacopy, that->data, LEDS_DATA_SLOTS*sizeof(int));
          xSemaphoreTake(that->newData, 1);     // consume newdata
          xSemaphoreGive(that->dataInUse);      // let data be set by others
          that->frame(datacopy);                // draw on strip
        }

      } 
      while(that->loop());

      LOGF("ANIM: %s end \n", that->name());
      that->release();
      vTaskDelete(NULL);
    }
    
    // identity
    String _name = "?"; 
    bool _loop = true;

    // output
    K32_ledstrip* _strip = NULL;
    int _size = 0;
    int _offset = 0; 

    // internal logic
    SemaphoreHandle_t newData;  
    SemaphoreHandle_t dataInUse;  
    bool _isRunning = false;

    // Animation
    TaskHandle_t animateHandle = NULL;
    int _fps = LEDS_ANIM8_FPS;
    unsigned long _stopAt = 0;
    SemaphoreHandle_t stop_lock;
    SemaphoreHandle_t wait_lock;

};



#endif