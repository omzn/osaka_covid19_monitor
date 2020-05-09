/* Smart COVID-19 Osaka Model monitor */

#include <M5Stack.h>

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <RTClib.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

#include "ntp.h"

#include "DSEG7Classic-BoldItalic16pt7b.h"

#include "board.h"

#define COVID_UNKNOWN_TH 10
#define COVID_POSITIVE_TH 7
#define COVID_BED_TH 60

#define DEG2RAD 0.0174532925

#define FONT_7SEG16PT M5.Lcd.setFont(&DSEG7Classic_BoldItalic16pt7b)
#define FONT_7SEG9PT M5.Lcd.setFont(&DSEG7Classic_BoldItalic9pt7b)
#define FONT_SANS9PT M5.Lcd.setFont(&FreeSans9pt7b)
#define FONT_SANS6PT M5.Lcd.setFont(&FreeSans6pt7b)

#define TFT_DARKRED M5.Lcd.color565(140, 0, 0)
#define TFT_DARKYELLOW M5.Lcd.color565(180, 140, 0)
#define TFT_DARKDARKYELLOW M5.Lcd.color565(60, 60, 0)

// WebServer webServer(80);
// NTP ntp("ntp.nict.jp");
// RTC_Millis rtc;
Preferences prefs;
WiFiManager wifimanager;
unsigned long etime;
int is_error = 0;
String payload;

float covid_unknown, covid_positive, covid_bed;
int covid_month, covid_day, covid_signalcolor;

// TFT_eSprite img(&M5Stack::Lcd);

#define USE_SERIAL Serial

String osakaPrefURL = "http://www.pref.osaka.lg.jp/default.html";

String getSubstringFromTo(String *src, String from, String to, int idx_clear) {
  static int eidx = 0;
  if (idx_clear) {
    eidx = 0;
  }
  int bidx = src->indexOf(from, eidx);
  eidx = src->indexOf(to, bidx);
  String v = src->substring(bidx + from.length(), eidx);
  return v;
}

void drawFrame() {
  M5.Lcd.drawLine(0, 0, 303, 0, FGCOLOR);
  M5.Lcd.drawLine(0, 1, 303, 1, FGCOLOR);
  M5.Lcd.drawLine(0, 2, 303, 2, FGCOLOR);

  M5.Lcd.drawLine(303, 0, 319, 16, FGCOLOR);
  M5.Lcd.drawLine(303, 1, 318, 16, FGCOLOR);
  M5.Lcd.drawLine(303, 2, 317, 16, FGCOLOR);

  M5.Lcd.drawLine(319, 16, 319, 239, FGCOLOR);
  M5.Lcd.drawLine(318, 16, 318, 239, FGCOLOR);
  M5.Lcd.drawLine(317, 16, 317, 239, FGCOLOR);

  M5.Lcd.drawLine(0, 0, 0, 239, FGCOLOR);
  M5.Lcd.drawLine(1, 0, 1, 239, FGCOLOR);
  M5.Lcd.drawLine(2, 0, 2, 239, FGCOLOR);

  M5.Lcd.drawLine(0, 237, 319, 237, FGCOLOR);
  M5.Lcd.drawLine(0, 238, 319, 238, FGCOLOR);
  M5.Lcd.drawLine(0, 239, 319, 239, FGCOLOR);

  for (int i = 0; i < 320; i += 12) {
    for (int j = 0; j < 6; j++) {
      M5.Lcd.drawLine(4 + i + j, 236, 10 + i + j, 227, FGCOLOR);
    }
  }

  M5.Lcd.pushImage(12, 11, panelWidth, panelHeight, p_file);
  M5.Lcd.pushImage(228, 11, panelWidth, panelHeight, p_result);

  M5.Lcd.setTextColor(FGCOLOR, BGCOLOR);
  FONT_SANS9PT;
  M5.Lcd.drawString("COVID-19", 120, 125);
  M5.Lcd.drawString("Osaka", 12, 54);
  M5.Lcd.drawString("model", 20, 72);
  FONT_SANS6PT;
  //  M5.Lcd.drawString("Osaka model", 12, 80);
}

void drawMagi(int color_up, int color_left, int color_right) {
  // 上
  M5.Lcd.drawLine(104, 13, 216, 13, FGCOLOR);
  M5.Lcd.drawLine(104, 14, 216, 14, FGCOLOR);
  M5.Lcd.drawLine(104, 15, 216, 15, FGCOLOR);

  M5.Lcd.drawLine(103, 13, 103, 85, FGCOLOR);
  M5.Lcd.drawLine(104, 13, 104, 85, FGCOLOR);
  M5.Lcd.drawLine(105, 13, 105, 85, FGCOLOR);

  M5.Lcd.drawLine(215, 13, 215, 85, FGCOLOR);
  M5.Lcd.drawLine(216, 13, 216, 85, FGCOLOR);
  M5.Lcd.drawLine(217, 13, 217, 85, FGCOLOR);

  M5.Lcd.fillRect(106, 16, 214 - 106 + 1, 85 - 16 + 1, color_up);

  M5.Lcd.fillTriangle(107, 86, 138, 109, 138, 86, color_up);

  M5.Lcd.drawLine(104, 84, 138, 110, FGCOLOR);
  M5.Lcd.drawLine(104, 85, 138, 111, FGCOLOR);
  M5.Lcd.drawLine(104, 86, 138, 112, FGCOLOR);

  M5.Lcd.drawLine(138, 110, 182, 110, FGCOLOR);
  M5.Lcd.drawLine(138, 111, 182, 111, FGCOLOR);
  M5.Lcd.drawLine(138, 112, 182, 112, FGCOLOR);

  M5.Lcd.fillRect(138, 83, 182 - 138 + 1, 109 - 83 + 1, color_up);

  M5.Lcd.fillTriangle(216, 86, 182, 109, 182, 86, color_up);

  M5.Lcd.drawLine(216, 84, 182, 110, FGCOLOR);
  M5.Lcd.drawLine(216, 85, 182, 111, FGCOLOR);
  M5.Lcd.drawLine(216, 86, 182, 112, FGCOLOR);

  if (color_up == FG_BLUE) {
    M5.Lcd.pushImage(120, 20, panelWidth, panelHeight, unknown_b);
  } else {
    M5.Lcd.pushImage(120, 20, panelWidth, panelHeight, unknown_r);
  }

  // 上→左
  M5.Lcd.drawLine(138, 110, 110, 129, FGCOLOR);
  M5.Lcd.drawLine(138, 111, 110, 130, FGCOLOR);
  M5.Lcd.drawLine(138, 112, 110, 131, FGCOLOR);

  // 左
  // ↓
  M5.Lcd.drawLine(11, 130, 11, 220, FGCOLOR);
  M5.Lcd.drawLine(12, 130, 12, 220, FGCOLOR);
  M5.Lcd.drawLine(13, 130, 13, 220, FGCOLOR);

  M5.Lcd.fillRect(14, 132, 110 - 14 + 1, 218 - 132 + 1, color_left);
  // →
  M5.Lcd.drawLine(12, 219, 140, 219, FGCOLOR);
  M5.Lcd.drawLine(12, 220, 140, 220, FGCOLOR);
  M5.Lcd.drawLine(12, 221, 140, 221, FGCOLOR);
  // →
  M5.Lcd.drawLine(12, 129, 110, 129, FGCOLOR);
  M5.Lcd.drawLine(12, 130, 110, 130, FGCOLOR);
  M5.Lcd.drawLine(12, 131, 110, 131, FGCOLOR);
  // 斜め
  M5.Lcd.drawLine(110, 129, 141, 160, FGCOLOR);
  M5.Lcd.drawLine(110, 130, 140, 160, FGCOLOR);
  M5.Lcd.drawLine(110, 131, 139, 160, FGCOLOR);

  M5.Lcd.fillRect(14, 160, 138 - 14 + 1, 218 - 160 + 1, color_left);

  M5.Lcd.drawLine(139, 160, 139, 220, FGCOLOR);
  M5.Lcd.drawLine(140, 160, 140, 220, FGCOLOR);
  M5.Lcd.drawLine(141, 160, 141, 220, FGCOLOR);

  M5.Lcd.fillTriangle(110, 132, 110, 160, 138, 160, color_left);

  if (color_left == FG_BLUE) {
    M5.Lcd.pushImage(17, 136, panelWidth, panelHeight, posi_b);
  } else {
    M5.Lcd.pushImage(17, 136, panelWidth, panelHeight, posi_r);
  }

  // 上→右
  M5.Lcd.drawLine(182, 110, 320 - 110, 129, FGCOLOR);
  M5.Lcd.drawLine(182, 111, 320 - 110, 130, FGCOLOR);
  M5.Lcd.drawLine(182, 112, 320 - 110, 131, FGCOLOR);

  //左→ 右
  M5.Lcd.drawLine(320 - 140, 159, 140, 159, FGCOLOR);
  M5.Lcd.drawLine(320 - 140, 160, 140, 160, FGCOLOR);
  M5.Lcd.drawLine(320 - 140, 161, 140, 161, FGCOLOR);

  // 右
  M5.Lcd.drawLine(320 - 11, 130, 320 - 11, 220, FGCOLOR);
  M5.Lcd.drawLine(320 - 12, 130, 320 - 12, 220, FGCOLOR);
  M5.Lcd.drawLine(320 - 13, 130, 320 - 13, 220, FGCOLOR);

  M5.Lcd.drawLine(320 - 12, 219, 320 - 140, 219, FGCOLOR);
  M5.Lcd.drawLine(320 - 12, 220, 320 - 140, 220, FGCOLOR);
  M5.Lcd.drawLine(320 - 12, 221, 320 - 140, 221, FGCOLOR);

  M5.Lcd.drawLine(320 - 12, 129, 320 - 110, 129, FGCOLOR);
  M5.Lcd.drawLine(320 - 12, 130, 320 - 110, 130, FGCOLOR);
  M5.Lcd.drawLine(320 - 12, 131, 320 - 110, 131, FGCOLOR);

  M5.Lcd.fillRect(320 - 110, 132, 110 - 14 + 1, 218 - 132 + 1, color_right);

  M5.Lcd.drawLine(320 - 110, 129, 320 - 141, 160, FGCOLOR);
  M5.Lcd.drawLine(320 - 110, 130, 320 - 140, 160, FGCOLOR);
  M5.Lcd.drawLine(320 - 110, 131, 320 - 139, 160, FGCOLOR);

  M5.Lcd.fillRect(320 - 138, 160, 138 - 14 + 1, 218 - 160 + 1, color_right);

  M5.Lcd.drawLine(320 - 139, 160, 320 - 139, 220, FGCOLOR);
  M5.Lcd.drawLine(320 - 140, 160, 320 - 140, 220, FGCOLOR);
  M5.Lcd.drawLine(320 - 141, 160, 320 - 141, 220, FGCOLOR);

  M5.Lcd.fillTriangle(320 - 110, 132, 320 - 110, 160, 320 - 138, 160,
                      color_right);

  if (color_right == FG_BLUE) {
    M5.Lcd.pushImage(220, 136, panelWidth, panelHeight, bed_b);
  } else {
    M5.Lcd.pushImage(220, 136, panelWidth, panelHeight, bed_r);
  }

  FONT_SANS9PT;
  M5.Lcd.drawString("2020-" + String(covid_month) + "-" + String(covid_day), 12,
                    100);

  if (covid_signalcolor == 1) {
    M5.Lcd.pushImage(228, 55, resultWidth, resultHeight, r_red);
  } else if (covid_signalcolor == 2) {
    M5.Lcd.pushImage(228, 55, resultWidth, resultHeight, r_yellow);
  } else if (covid_signalcolor == 3) {
    M5.Lcd.pushImage(228, 55, resultWidth, resultHeight, r_green);
  }

  FONT_7SEG16PT;
  M5.Lcd.setTextColor(FGCOLOR, color_up);
  M5.Lcd.drawFloat(covid_unknown, 1, 132, 61);

  M5.Lcd.setTextColor(FGCOLOR, color_left);
  M5.Lcd.drawFloat(covid_positive, 1, 50, 180);

  M5.Lcd.setTextColor(FGCOLOR, color_right);
  M5.Lcd.drawFloat(covid_bed, 1, 200, 180);

  FONT_SANS9PT;
  M5.Lcd.drawString("%", 120, 195);
  M5.Lcd.drawString("%", 285, 195);
}

void getCovidOsakaStatus() {
  HTTPClient http;

  USE_SERIAL.print("[HTTP] begin...\n");

  // http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
  http.begin(osakaPrefURL); // HTTP

  USE_SERIAL.print("[HTTP] GET...\n");
  int httpCode = http.GET();

  if (httpCode > 0) {
    USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      payload = http.getString();

      // 色(90 46) -> この直前2バイトが 赤(90 d4)，黄(89 a9)，緑(97 ce)
      String signalstr = getSubstringFromTo(&payload, "<h2>", "</h2>", 1);

      if (signalstr.charAt(signalstr.length() - 3) == 0xd4) {
        covid_signalcolor = 1;
      } else if (signalstr.charAt(signalstr.length() - 3) == 0xa9) {
        covid_signalcolor = 2;
      } else if (signalstr.charAt(signalstr.length() - 3) == 0xce) {
        covid_signalcolor = 3;
      } else {
        covid_signalcolor = 0;
      }
      getSubstringFromTo(&payload, "rowspan=\"2\"", "WIDTH", 0);
      String date = getSubstringFromTo(&payload, ";\">", "</br>", 0);
      USE_SERIAL.println(date);
      String month, day;
      int mdcount = 0;
      for (int i = 0; i <= date.length(); i++) {
        if (date.charAt(i) >= '0' && date.charAt(i) <= '9') {
          if (mdcount == 0) {
            month = month + date.charAt(i);
          } else {
            day = day + date.charAt(i);
          }
        } else {
          mdcount++;
        }
      }
      covid_month = month.toInt();
      covid_day = day.toInt();

      getSubstringFromTo(&payload, "#ccccff", "class", 0);
      String rawstr = getSubstringFromTo(&payload, ";\">", "</td>", 0);
      String unknown;
      for (int i = 0; i <= rawstr.length(); i++) {
        if (rawstr.charAt(i) >= '0' && rawstr.charAt(i) <= '9' ||
            rawstr.charAt(i) == '.') {
          unknown = unknown + rawstr.charAt(i);
        }
      }
      USE_SERIAL.println(unknown);
      covid_unknown = unknown.toFloat();

      getSubstringFromTo(&payload, "#ccccff", "class", 0);
      rawstr = getSubstringFromTo(&payload, ";\">", "</td>", 0);
      String posi;
      for (int i = 0; i <= rawstr.length(); i++) {
        if (rawstr.charAt(i) >= '0' && rawstr.charAt(i) <= '9' ||
            rawstr.charAt(i) == '.') {
          posi = posi + rawstr.charAt(i);
        }
      }
      USE_SERIAL.println(posi);
      covid_positive = posi.toFloat();

      getSubstringFromTo(&payload, "#ccccff", "class", 0);
      rawstr = getSubstringFromTo(&payload, ";\">", "</td>", 0);
      String bed;
      for (int i = 0; i <= rawstr.length(); i++) {
        if (rawstr.charAt(i) >= '0' && rawstr.charAt(i) <= '9' ||
            rawstr.charAt(i) == '.') {
          bed = bed + rawstr.charAt(i);
        }
      }
      USE_SERIAL.println(bed);
      covid_bed = bed.toFloat();
    }
  } else {
    USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n",
                      http.errorToString(httpCode).c_str());
  }

  http.end();
}

bool is_power_control;

void setup() {
  M5.begin();
  Serial.begin(115200);
  Serial.println("Booting");
  rtc.begin(DateTime(2019, 5, 1, 0, 0, 0));
  prefs.begin("covid19osaka", false);

  M5.Power.begin();
  if (!M5.Power.canControl()) {
    is_power_control = false;
  }
  if (is_power_control) {
    M5.Power.setPowerVin(false);
    //  M5.Power.setLowPowerShutdownTime(ShutdownTime::SHUTDOWNTIME_64S);
  }

  wifimanager.autoConnect("covidmonitor");
  ArduinoOTA.setHostname("covidmonitor");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
        // using SPIFFS.end()
        Serial.println("Start updating " + type);
        pinMode(2, OUTPUT);
      })
      .onEnd([]() {
        digitalWrite(2, LOW);
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        //        digitalWrite(2, (progress / (total / 100)) % 2);
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });
  ArduinoOTA.begin();

  M5.Lcd.fillScreen(BGCOLOR);
  M5.Lcd.setSwapBytes(true);
  // Draw the icons
  FONT_7SEG16PT;
  M5.Lcd.setTextColor(FGCOLOR, BGCOLOR);

  M5.Lcd.fillScreen(BGCOLOR);
  drawFrame();
  getCovidOsakaStatus();
  drawMagi(covid_unknown >= COVID_UNKNOWN_TH ? FG_RED : FG_BLUE,
           covid_positive >= COVID_POSITIVE_TH ? FG_RED : FG_BLUE,
           covid_bed >= COVID_BED_TH ? FG_RED : FG_BLUE);

  is_error = 1;
  //  ntp.begin();
  //  webServer.begin();
  //  uint32_t epoch = ntp.getTime();
  //  if (epoch > 0) {
  //    rtc.adjust(DateTime(epoch + SECONDS_UTC_TO_JST));
  //  }
  etime = millis();
}

void loop() {

  //  webServer.handleClient();
  ArduinoOTA.handle();

  if (millis() > etime + 60 * 60 * 1000) {
    etime = millis();
    M5.Lcd.fillScreen(BGCOLOR);
    drawFrame();
    getCovidOsakaStatus();
    drawMagi(covid_unknown >= COVID_UNKNOWN_TH ? FG_RED : FG_BLUE,
             covid_positive >= COVID_POSITIVE_TH ? FG_RED : FG_BLUE,
             covid_bed >= COVID_BED_TH ? FG_RED : FG_BLUE);
  }
  delay(10);
}