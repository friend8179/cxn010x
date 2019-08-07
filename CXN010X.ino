#include <Wire.h>
#include <time.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <Ticker.h> 

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
  
#define SSID "SCatGFW"
#define WIFI_KEY "31415926"

Ticker blinker;

CXNProjector projector;
CXNProjector_State last_state;
ControlServer server(&projector);

bool shouldRefreshScreen = false;
bool shouldRefreshHeathyStatus = false;

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

void setupWifi() {
  Serial.printf("Connecting to %s ", SSID);
  WiFi.begin(SSID, WIFI_KEY);
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
  blinker.attach(5, setShouldRefreshScreen);
  blinker.attach(120, [] { shouldRefreshHeathyStatus = true; });
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
    Serial.println("refresh screen");
    refreshScreen();
  }

  if (shouldRefreshHeathyStatus && projector.GetState() != STATE_POWER_OFF) {
    shouldRefreshHeathyStatus = false;
    projector.GetTemperature(refreshTemperatureCallback);
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
    if (projector.GetTemperatureValue() > 0) {
      u8g2.drawStr(0, 15, (String("T: ") + projector.GetTemperatureValue() + "\176C").c_str());
    }
    //u8g2.drawStr(0, 15, (String("LAST CMD: ") + server.getLastCommand()).c_str());
    
  } while (u8g2.nextPage());
}

void refreshTemperatureCallback(uint8_t temp) {
   Serial.print("temp: ");
   Serial.println(temp);
}

void handleButtonPress() {
  CXNProjector_State state = projector.GetState();
  if (STATE_POWER_OFF == projector.GetState()) {
    Serial.print("power on\n");
    projector.PowerOn();
  } else {
    Serial.print("shutdown\n");
    projector.Shutdown(false);
    blinker.once(5, [] {
      if (projector.GetState() != STATE_POWER_OFF) {
        Serial.print("Force poweroff");
        projector.PowerOff();
      }
    });
    
  }
  delay(100);
  setShouldRefreshScreen();
}
