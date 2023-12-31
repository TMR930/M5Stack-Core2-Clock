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

const int ALARM_START_HOUR = 8;
const int ALARM_END_HOUR = 21;

int beep_volume = 3;
const int TEXT_COLOR = TFT_PURPLE;

float temp = 0.00;
int humd = 0;
float pressure = 0.00;

int old_ss = 00;

const int DISPLAY_W = 320;
const int DISPLAY_H = 240;

const int TIME_POS_Y = 40;
const int ENV_POS_Y = 130;

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
    M5.Lcd.setTextColor(TEXT_COLOR, TFT_BLACK);
  } else {
    M5.Lcd.print(":");
  }
}

void drawTime(int hh, int mm, int ss) {
  M5.Lcd.setTextColor(TEXT_COLOR, TFT_BLACK);
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(52, TIME_POS_Y);
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
  M5.Lcd.drawString(text, ((DISPLAY_W / 3) - 50), (DISPLAY_H - 20));
}

void envDisplay() {
  pressure = qmp6988.calcPressure();
  if (sht30.get() == 0) {
    temp = sht30.cTemp;
    humd = sht30.humidity;
  } else {
    temp = 0, humd = 0;
  }
  M5.Lcd.setTextColor(TEXT_COLOR, TFT_BLACK);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextSize(1);

  M5.Lcd.drawFloat(temp, 1, (DISPLAY_W / 4), ENV_POS_Y);
  M5.Lcd.drawString(String(humd), ((DISPLAY_W / 4) * 3), ENV_POS_Y);
}

void drawEnvTextSetup() {
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setFreeFont(&unicode_24px);
  M5.Lcd.drawString("℃", ((DISPLAY_W / 4) + 70), (ENV_POS_Y + 30), 1);
  M5.Lcd.drawString("％", (((DISPLAY_W / 4) * 3) + 50), (ENV_POS_Y + 30), 1);
}

void drawSecondHandBar(int ss) {
  if (ss == 00) {
    M5.Lcd.fillRect(0, (DISPLAY_H - 10), DISPLAY_W, 10, TFT_BLACK);
  }
  M5.Lcd.fillRect(0, (DISPLAY_H - 10), (ss * 5.3), 10, TEXT_COLOR);
  M5.Lcd.drawRect(0, (DISPLAY_H - 10), DISPLAY_W, 10, TEXT_COLOR);

  for (int i = 1; i < 6; i++) {
    M5.Lcd.drawFastVLine((DISPLAY_W / 6 * i), (DISPLAY_H - 10), 10, TEXT_COLOR);
  }
}

void setup(void) {
  M5.begin();
  Wire.begin();
  qmp6988.init();

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TEXT_COLOR, TFT_BLACK);

  // connect to WiFi
  M5.Lcd.printf("Connecting to %s \n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("CONNECTED!");
  M5.Spk.PlaySound(previewR, sizeof(previewR));

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
  // second hand bar
  drawSecondHandBar(ss);

  // btnAdisplayText("Vol:" + String(beep_volume));
  // btnAdisplayText("beep");
  // Set alarm volume
  // if (M5.BtnA.isPressed()) {
  //   ++beep_volume;
  //   if (beep_volume > 6) beep_volume = 0;
  //   // btnAdisplayText("Vol:" + String(beep_volume));
  //   beep();
  // }

  if (M5.BtnA.isPressed()) {
    M5.Axp.SetLDOEnable(3, true);  // Open the vibration.
    delay(100);
    M5.Axp.SetLDOEnable(3, false);
    delay(100);
    M5.Spk.PlaySound(previewR, sizeof(previewR));
  }

  // Set time signal
  if (hh >= ALARM_START_HOUR && hh <= ALARM_END_HOUR) {
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
