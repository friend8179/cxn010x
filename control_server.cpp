#include <ESP8266WiFi.h>
#include <ESP8266Webserver.h>
#include <FS.h>
#include <sstream>

#include "cxn010x.h"
#include "control_server.h"
#include "HexDump.h"

ControlServer::ControlServer(CXNProjector* projector_ptr) :
  projector(projector_ptr) {
  server = new ESP8266WebServer(80);
  SPIFFS.begin();
}

ControlServer::~ControlServer() {
  delete server;
}


String ControlServer::getLastCommand() {
  return "";//lastCommand;
}

bool ControlServer::handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.html";
  }
  String contentType = "text/html";
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server->streamFile(file, contentType);
    file.close();
    return true;
  } 
  return false;
}

void ControlServer::handleDefault() {
  if (!handleFileRead(server->uri())) {
    server->send(404, "text/plain", "FileNotFound");
  }
}

void ControlServer::handleCommand() {
  String response = "";
  String cmd = server->arg("cmd");
  if (cmd == "") {
    response += "no command\n";
  } else {
    long val = server->arg("val").toInt();
    Serial.print(cmd + ",");
    Serial.println(val);

    response += "command = " + cmd + "\n";
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
      projector->SetBrightness(val);
    } else if (cmd == "contrast") {
      projector->SetContrast(val);
    } else if (cmd == "sharpness") {
      projector->SetSharpness(val);
    } else if (cmd == "saturat") {
      projector->SetSaturat(val);
    } else if (cmd == "get_alignment") {
      projector->GetOpticalAlignment();
    } else if (cmd == "get_biphase") {
      projector->GetBiphase();
    } else if (cmd == "get_temperature") {
      projector->GetTemperature([](uint8_t v){Serial.println(v);});
    } else if (cmd == "get_time") {
      projector->GetCumulativeOperatingTime();
    } 
  }

  response += String("projector.state = ") + String(projector->GetState());
  response += "\n";

  server->send(200, "text/plain", response);
  setShouldRefreshScreen();
}

void ControlServer::handleAdjust() {
  String response = "";
  String cmd = server->arg("cmd");
  if (cmd == "") {
    response += "no command\n";
  } else {
    String val = server->arg("val");
    Serial.print(cmd + ",");
    Serial.println(val);

    response += "command = " + cmd + "\n";
    response += "val = " + val + "\n";

    char* inputChars = const_cast<char*>(val.c_str());
    int8_t buffer[32];
    size_t size = 0; 
    char* tmp;
    tmp = strtok(inputChars, " ");    
    do {
      buffer[size++] = String(tmp).toInt();
    } while ((tmp = strtok(NULL, " ")));
    Serial.print("size: ");
    Serial.println(size);
 
    if (cmd == "set_alignment") {      
      projector->SetOpticalAlignment(buffer, size);
    }
  }

  server->send(200, "text/plain", response);
  setShouldRefreshScreen();
}

String dataDumpToHex(uint8_t* data) {
  String result = "";
  char buffer[4];
  for (size_t j = 0; j < data[1] + 2; j++){
    sprintf(buffer, "%02X ", data[j]);
    result += buffer;
  }
  return result;
}

String dataDumpToInt(uint8_t* data) {
  String result = "";
  char buffer[4];
  for (size_t j = 0; j < data[1] + 2; j++){
    sprintf(buffer, "%d ", (int8_t)data[j]);
    result += buffer;
  }
  return result;
}

void ControlServer::handleNotify() {
  uint8_t *data = projector->getData();
  String response = "Last notify:" + dataDumpToHex(data) + "\n";  
  response += "Last notify(int):" + dataDumpToInt(data) + "\n";
  server->send(200, "text/plain", response);
  setShouldRefreshScreen();
}

void ControlServer::handleRequest() {
  server->handleClient();
}

void ControlServer::setup() {
  server->on("/cmd", std::bind(&ControlServer::handleCommand, this));
  server->on("/adjustcmd", std::bind(&ControlServer::handleAdjust, this));
  server->on("/notify", std::bind(&ControlServer::handleNotify, this));
  server->onNotFound(std::bind(&ControlServer::handleDefault, this));
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
