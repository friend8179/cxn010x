#include <ESP8266WiFi.h>
#include <ESP8266Webserver.h>

#include "cxn010x.h"
#include "control_server.h"

ControlServer::ControlServer(CXNProjector* projector_ptr) :
  projector(projector_ptr) {
  server = new ESP8266WebServer(80);
}

ControlServer::~ControlServer() {
  delete server;
}


String ControlServer::getLastCommand() {
  return "";//lastCommand;
}

void ControlServer::handleRoot() { 
  String response = "";
  String cmd = "";//server->arg("cmd");
  if (cmd == "") {
     response += "<p>no command</p>";   
  } else {
    long val = 0;//server->arg("val").toInt();
    Serial.print(cmd);
    Serial.print(",");
    Serial.println(val);
    
    response += "<p>command = " + cmd + "</p>";
    //lastCommand = cmd;
    if (cmd == "reboot") {
      projector->Shutdown(true);
    } else if (cmd == "start_input") {
      projector->StartInput();
    } else if (cmd == "stop_input") {
      projector->StopInput();
    } else if (cmd == "get_quality") {
      projector->GetAllPictureQualityInfo();
    } else if (cmd == "flip") {
      projector->SetFlip(val);
    } else if (cmd == "pan") {
      projector->SetPan(val);
    } else if (cmd == "tilt") {
      projector->SetTilt(val);
    } else if (cmd == "brightness") {
      projector->SetLight(val);
    } else if (cmd == "contrast") {
      projector->SetContrast(val);
    } else if (cmd == "sharpness") {
      projector->SetSharp(val);
    } else if (cmd == "saturat") {
      projector->SetSaturat(val);
    } else if (cmd == "get_alignment") {
      projector->GetOpticalAlignment();
    } else if (cmd == "get_biphase") {
      projector->GetBiphase();
    }   
  }
 
  response += String("<p>projector.state = ") + String(projector->GetState());
  response += "</p>";
  
  server->send(200, "text/html", response);
  setShouldRefreshScreen();
}

void ControlServer::handleAdjust() { 
  String response = "todo";
  server->send(200, "text/html", response);
  setShouldRefreshScreen();
}

void ControlServer::handleRequest() {
  server->handleClient();
}

void ControlServer::setup() {
  server->on("/", std::bind(&ControlServer::handleRoot, this)); 
  server->on("/adjust", std::bind(&ControlServer::handleAdjust, this)); 
  server->begin();
  Serial.printf("Web server on %s\n", WiFi.localIP().toString().c_str());
}

void ControlServer::setShouldRefreshScreen() {
  shouldRefreshScreen = true;
}

bool ControlServer::getShouldRefreshScreen() {
  if (shouldRefreshScreen) {
    shouldRefreshScreen = false;
    return true;
  }
  return false;
}
