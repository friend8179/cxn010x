#include <Wire.h>
#include <time.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "cxn010x.h"
#include "HexDump.h"

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

//WiFiServer server(80);
ESP8266WebServer server(80);  

CXNProjector projector;
CXNProjector_State last_state;

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
  
  Wire.begin(); //初始化I2C
  u8g2.begin();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(0, 24, "Hello World!");
  } while (u8g2.nextPage());

  setupWifi();

  server.on("/", handleRoot); 
  server.begin();
  Serial.printf("Web server on %s\n", WiFi.localIP().toString().c_str());

  Serial.print("projector power off\n");
  projector.PowerOff();

}

void loop() {  
  if (digitalRead(CMD_REQ_PIN) == HIGH) {
    delay(10); 
    while (HIGH == digitalRead(CMD_REQ_PIN)) {
      projector.OnNotify();
    }
    deferRefreshScreen();
  } 
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(1);
    if (digitalRead(BUTTON_PIN) == LOW) {
      handleButtonPress();

    }
  }
  server.handleClient();
  if (shouldRefreshScreen) {
    refreshScreen();
  }
}

void deferRefreshScreen() {
  shouldRefreshScreen = true;
}

String lastCommand = "";
void refreshScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_tinytim_tr); // 5px height
    u8g2.drawStr(70, 6, WiFi.localIP().toString().c_str());

    u8g2.setFont(u8g2_font_5x7_tr); // 6px height
    u8g2.drawStr(0, 7, (String("STATE: ") + projector.GetState()).c_str());
    u8g2.drawStr(0, 15, (String("LAST CMD: ") + lastCommand).c_str());
    
  } while (u8g2.nextPage());
  shouldRefreshScreen = false;
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
  deferRefreshScreen();
}

void handleRoot() { 
  String response = "";
  String cmd = server.arg("cmd");
  if (cmd == "") {
     response += "<p>no command</p>";   
  } else {
    response += "<p>command = " + cmd + "</p>";
    lastCommand = cmd;
    if (cmd == "reboot") {
      projector.Shutdown(true);
    } else if (cmd == "start_input") {
      projector.StartInput();
    } else if (cmd == "stop_input") {
      projector.StopInput();
    } else if (cmd == "save_config") {
        // 上下文(保存)
      projector.SaveConfig();
    }
    /*break;
      case "pan_left": // 左
        projector.SetPan(-1);
        break;
      case "pan_top": // 上
        projector.SetTilt(1);
        break;
      case "pan_bottom": // 下
        projector.SetTilt(-1);
        break;
      case "flip": // OK
        projector.SetFlip();
        break;
      case "vol_up": // VOL+ 亮度+
        projector.SetLight(+1);
        break;
      case "vol_down": // VOL- 亮度-
        projector.SetLight(-1);
        break;
      case "vol_mute"://Mute 静音
        //projector.m_Brightness = 0;
        projector.SetLight(0);
        break;
      case "save_config": // 上下文(保存)
        projector.SaveConfig();
        break;
      case "reset": //返回 恢复所有位置信息
        //projector.m_Pan = projector.m_Tilt = projector.m_Flip = 0;
        projector.SetVideoPosition();
        break;
    }*/
  }
 
  response += "<p>projector.state = " + projector.GetState();
  response += "</p>";
  
  server.send(200, "text/html", response);
  deferRefreshScreen();
}
