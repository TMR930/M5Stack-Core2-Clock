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
int humid = 0;
float pressure = 0.00;

int old_ss = 00;

const int DISPLAY_W = 320;
const int DISPLAY_H = 240;

const int TIME_POS_Y = 5;
const int ENV_POS_Y = 60;

const int GRAPH_W = 280;
const int GRAPH_H = 100;
const int GRAPH_OFFSET_Y = 120;

// Forward declaration needed for IDE 1.6.x
static uint8_t conv2d(const char* p);

// Get H, M, S from compile time
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3),
        ss = conv2d(__TIME__ + 6);

SHT3X sht30;
QMP6988 qmp6988;

TFT_eSprite graph = TFT_eSprite(&M5.Lcd);

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

void envDisplay() {
  pressure = qmp6988.calcPressure();
  if (sht30.get() == 0) {
    temp = sht30.cTemp;
    humid = sht30.humidity;
  } else {
    temp = 0, humid = 0;
  }
  M5.Lcd.setTextColor(TEXT_COLOR, TFT_BLACK);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.setTextFont(7);
  M5.Lcd.setTextSize(1);

  M5.Lcd.drawFloat(temp, 1, (DISPLAY_W / 4), ENV_POS_Y);
  M5.Lcd.drawString(String(humid), ((DISPLAY_W / 4) * 3), ENV_POS_Y);

  // Placing Sprites for Graphs
  graph.pushSprite(((DISPLAY_W - GRAPH_W) / 2 - 1), (15 + GRAPH_OFFSET_Y));
  graph.scroll(-1, 0);

  // Draw a tick line
  for (int y = 0; y <= GRAPH_H; y += GRAPH_H / 10)
    graph.drawPixel((GRAPH_W - 1), (y), TFT_DARKGREEN);
  M5.Lcd.setTextFont(0);
  M5.Lcd.setTextSize(1);

  // Scale for temperature
  M5.Lcd.setTextColor(TFT_YELLOW);
  M5.Lcd.setCursor(0, GRAPH_OFFSET_Y);
  M5.Lcd.printf("Temp");
  char tempScale[6][4] = {" 40", " 30", " 20", " 10", "  0", "-10"};
  for (int i = 0; i < 6; i++) {
    M5.Lcd.setCursor(0, 12 + i * GRAPH_H / 10 * 2 + GRAPH_OFFSET_Y);
    M5.Lcd.printf("%s", tempScale[i]);
  }

  // Scale for humidity
  M5.Lcd.setTextColor(TFT_BLUE);
  M5.Lcd.setCursor(DISPLAY_W - 30, GRAPH_OFFSET_Y);
  M5.Lcd.printf("Humid");
  char humidScale[6][4] = {"100", "80", "60", "40", "20", "0"};
  for (int i = 0; i < 6; i++) {
    M5.Lcd.setCursor(GRAPH_W + (DISPLAY_W - GRAPH_W) / 2 + 1,
                     12 + i * GRAPH_H / 10 * 2 + GRAPH_OFFSET_Y);
    M5.Lcd.printf("%s", humidScale[i]);
  }

  // Draw a temperature graph
  if (temp > 40.0) temp = 40.0;
  if (temp < -10.0) temp = -10.0;
  float tc = -2.0 * (temp - 40.0);
  graph.drawFastVLine(GRAPH_W - 1, (int)tc, 2, TFT_YELLOW);

  // Draw a humidity graph
  float hc = -1.0 * (humid - 100.0);
  graph.drawFastVLine(GRAPH_W - 1, (int)hc, 2, TFT_BLUE);
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

  // Generating Sprites for Graphs
  graph.setColorDepth(8);
  graph.createSprite(GRAPH_W, GRAPH_H + 1);
  graph.fillSprite(TFT_BLACK);
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
  // drawSecondHandBar(ss);

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
