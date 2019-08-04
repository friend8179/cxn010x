#include <Wire.h>
#include <time.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>

#include "cxn010x.h"
#include "HexDump.h"
#include "control_server.h"

/*
 * Airmon ESP8266 base
 * IIC  SCL-5  SDA-4
 * LED_PIN  GPIO2 (2)
*/

#define LED_PIN           2  
#define CMD_REQ_PIN       13
#define BUTTON_PIN        12
#define CXNProjector_POWER_PIN  15

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);
  
const char* ssid = "SCatGFW";
const char* password = "31415926";

CXNProjector projector;
CXNProjector_State last_state;
ControlServer server(&projector);

bool shouldRefreshScreen = false;

void updateStatus() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN)); 
  
  CXNProjector_State state = projector.GetState();
  if (last_state != state) {
    Serial.print("state_change:");
    Serial.print(state);
    Serial.print("\n");
    last_state = state;
  }
}

void setupWifi() {
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  
  refreshScreen();
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(CMD_REQ_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(CXNProjector_POWER_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println("Hello~");
  
  Wire.begin();
  delay(10);
  
  u8g2.begin();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(0, 24, "Hello World!");
  } while (u8g2.nextPage());

  setupWifi();
  server.setup();

  Serial.print("projector power off\n");
  projector.PowerOff();
}

void handleButtonPress();

void loop() {  
  if (digitalRead(CMD_REQ_PIN) == HIGH) {
    delay(10); 
    while (HIGH == digitalRead(CMD_REQ_PIN)) {
      projector.OnNotify();
    }
    setShouldRefreshScreen();
  } 
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(1);
    if (digitalRead(BUTTON_PIN) == LOW) {
      handleButtonPress();
    }
  }
  
  server.handleRequest();
  
  if (server.getShouldRefreshScreen() || getShouldRefreshScreen()) {
    refreshScreen();
  }
}

void refreshScreen() {
  u8g2.begin();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_tinytim_tr); // 5px height
    u8g2.drawStr(70, 6, WiFi.localIP().toString().c_str());

    u8g2.setFont(u8g2_font_5x7_tr); // 6px height
    u8g2.drawStr(0, 7, (String("STATE: ") + projector.GetState()).c_str());
    if (digitalRead(CXNProjector_POWER_PIN) == HIGH) {
      u8g2.drawStr(110, 15, "ON");
    } else {
      u8g2.drawStr(110, 15, "OFF");
    }
    //u8g2.drawStr(0, 15, (String("LAST CMD: ") + server.getLastCommand()).c_str());
    
  } while (u8g2.nextPage());
}

void handleButtonPress() {
  CXNProjector_State state = projector.GetState();
  if (STATE_POWER_OFF == projector.GetState()) {
    Serial.print("power on\n");
    projector.PowerOn();
  } else {
    Serial.print("shutdown\n");
    projector.Shutdown(false);
  }
  delay(1000);
  setShouldRefreshScreen();
}

void setShouldRefreshScreen() {
  shouldRefreshScreen = true;
}

bool getShouldRefreshScreen() {
  if (shouldRefreshScreen) {
    shouldRefreshScreen = false;
    return true;
  }
  return false;
}
