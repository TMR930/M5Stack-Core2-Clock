#include <M5Core2.h>
#include <WiFi.h>

#include "CUF_24px.h"
#include "M5_ENV.h"
#include "melody.h"
#include "time.h"

extern const unsigned char previewR[120264];

const char* ssid = "yourSSID";
const char* password = "yourPASSWORD";

const char* ntp_server = "ntp.jst.mfeed.ad.jp";
const long gmt_offset_sec = 9 * 3600;
const int daylight_offset_sec = 0;

const int alarm_start_hour = 8;
const int alarm_end_hour = 21;

int beep_volume = 3;
int text_color = TFT_PURPLE;

float temp = 0.00;
int humd = 0;
float pressure = 0.00;

int old_ss = 00;

const int display_w = 320;
const int display_h = 240;

// Forward declaration needed for IDE 1.6.x
static uint8_t conv2d(const char* p);

// Get H, M, S from compile time
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3),
        ss = conv2d(__TIME__ + 6);

SHT3X sht30;
QMP6988 qmp6988;

void drawTimeTextSet(int time) {
  if (time < 10) M5.Lcd.print('0');
  M5.Lcd.print(time);
}

void drawColon(int sec) {
  if (sec % 2) {  // Flash the colons on/off
    M5.Lcd.setTextColor(0x0001, TFT_BLACK);
    M5.Lcd.print(":");
    M5.Lcd.setTextColor(text_color, TFT_BLACK);
  } else {
    M5.Lcd.print(":");
  }
}

void drawTime(int hh, int mm, int ss) {
  M5.Lcd.setTextColor(text_color, TFT_BLACK);
  // M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(52, 140);
  drawTimeTextSet(hh);  // Draw hours
  drawColon(ss);
  drawTimeTextSet(mm);  // Draw minutes
  drawColon(ss);
  drawTimeTextSet(ss);  // Draw seconds
}

void btnAdisplayText(String text) {
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(2);
  // M5.Lcd.drawString(text, 68, 220);
  M5.Lcd.drawString(text, ((display_w / 3) - 50), (display_h - 20));
}

void envDisplay() {
  pressure = qmp6988.calcPressure();
  if (sht30.get() == 0) {
    temp = sht30.cTemp;
    humd = sht30.humidity;
  } else {
    temp = 0, humd = 0;
  }
  M5.Lcd.setTextColor(text_color, TFT_BLACK);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextSize(1);

  M5.Lcd.drawFloat(temp, 1, (display_w / 4), 50);
  M5.Lcd.drawString(String(humd), ((display_w / 4) * 3), 50);
}

void drawEnvTextSetup() {
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setFreeFont(&unicode_24px);
  M5.Lcd.drawString("℃", ((display_w / 4) + 70), 80, 1);
  M5.Lcd.drawString("％", (((display_w / 4) * 3) + 50), 80, 1);
}

void setup(void) {
  M5.begin();
  Wire.begin();
  qmp6988.init();

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(text_color, TFT_BLACK);

  // connect to WiFi
  M5.Lcd.printf("Connecting to %s \n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("CONNECTED!");

  // init and get the time
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.println("Failed to obtain time");
    return;
  }
  // disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  M5.Lcd.clearDisplay();

  drawEnvTextSetup();
}

void loop() {
  M5.update();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.println("Failed to obtain time");
    return;
  }
  hh = timeinfo.tm_hour;
  mm = timeinfo.tm_min;
  ss = timeinfo.tm_sec;

  // Show on display
  drawTime(hh, mm, ss);
  // btnAdisplayText("Vol:" + String(beep_volume));
  btnAdisplayText("beep");
  // Set alarm volume
  // if (M5.BtnA.isPressed()) {
  //   ++beep_volume;
  //   if (beep_volume > 6) beep_volume = 0;
  //   // btnAdisplayText("Vol:" + String(beep_volume));
  //   beep();
  // }
  if (M5.BtnA.isPressed()) {
    M5.Axp.SetLDOEnable(3, true);  // Open the vibration.   开启震动马达
    delay(100);
    M5.Axp.SetLDOEnable(3, false);
    delay(100);
    M5.Spk.PlaySound(previewR, sizeof(previewR));
  }

  // Set time signal
  if (hh >= alarm_start_hour && hh <= alarm_end_hour) {
    if ((mm == 00 && ss == 00)) {
      M5.Spk.PlaySound(previewR, sizeof(previewR));
    }
  }

  if (ss != old_ss) {
    envDisplay();
  }
  old_ss = ss;
}

// Function to extract numbers from compile time string
static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9') v = *p - '0';
  return 10 * v + *++p - '0';
}
