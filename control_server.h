#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
//#include "cxn010x.h"

#pragma pack(1)
class ControlServer {

public:
  ControlServer(CXNProjector* projector);
  ~ControlServer(); 

  void setup();
  void handleRequest();
  
  String getLastCommand();
  bool getShouldRefreshScreen();

private:
  ESP8266WebServer* server;
  CXNProjector* projector;

  //String lastCommand;
  bool shouldRefreshScreen = false;


  void handleRoot(); 
  void handleAdjust(); 
  
  void setShouldRefreshScreen();
  
};

#pragma pop() 
