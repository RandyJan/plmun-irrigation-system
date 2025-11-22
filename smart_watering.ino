#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BLYNK_TEMPLATE_ID "TMPL6FDfnVPlW"
#define BLYNK_TEMPLATE_NAME "SPIS THESIS"
#define BLYNK_AUTH_TOKEN "aVLJS6_COot9QYTsMxkz9dUrTpalOwGX"

const char *ssid = "wifime";
const char *password = "12345678";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MOISTURE_PIN_A 34
#define MOISTURE_PIN_C 32

#define RELAY_PIN_A 2
#define RELAY_PIN_C 16

int autoStart = 0;
int autoStop = 0;
bool forceWater = false;
bool scheduleActive = false;

int centerX(String text, int textSize = 1)
{
  int textWidth = text.length() * 6 * textSize;
  return (128 - textWidth) / 2;
}

BLYNK_CONNECTED()
{
  Blynk.sendInternal("rtc_sync");
}

BLYNK_WRITE(V3)
{
  int value = param.asInt();
  forceWater = (value == 1);
}

BLYNK_WRITE(V4)
{
  TimeInputParam t(param);
  if (t.hasStartTime() && t.hasStopTime())
  {
    autoStart = t.getStartHour() * 3600 + t.getStartMinute() * 60;
    autoStop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
    scheduleActive = true;
  }
  else
  {
    scheduleActive = false;
  }
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  Blynk.sendInternal("rtc_sync");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  pinMode(RELAY_PIN_A, OUTPUT);
  pinMode(RELAY_PIN_C, OUTPUT);

  digitalWrite(RELAY_PIN_A, LOW);
  digitalWrite(RELAY_PIN_C, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int xMsg = centerX("WiFi Connected!");
  display.setCursor(xMsg, 0);
  display.println("WiFi Connected!");

  String ipStr = WiFi.localIP().toString();
  int xIP = centerX(ipStr);
  display.setCursor(xIP, 20);
  display.println(ipStr);

  display.display();
  delay(2500);
}

void loop()
{
  Blynk.run();

  int sensorA = analogRead(MOISTURE_PIN_A);
  int sensorC = analogRead(MOISTURE_PIN_C);

  int moistureA = map(sensorA, 4095, 1500, 0, 100);
  int moistureC = map(sensorC, 4095, 1500, 0, 100);

  moistureA = constrain(moistureA, 0, 100);
  moistureC = constrain(moistureC, 0, 100);

  Blynk.virtualWrite(V0, moistureA);
  Blynk.virtualWrite(V2, moistureC);

  if (moistureA < 20)
    Blynk.logEvent("SEND_NOTIFICATION", "Moisture A is LOW!");
  if (moistureC < 20)
    Blynk.logEvent("SEND_NOTIFICATION", "Moisture C is LOW!");

  bool manualA = (moistureA < 20);
  bool manualC = (moistureC < 20);

  time_t nowSec = hour() * 3600 + minute() * 60 + second();
  bool autoWater = scheduleActive && nowSec >= autoStart && nowSec <= autoStop;

  bool finalWaterStatus = forceWater || autoWater || manualA || manualC;

  digitalWrite(RELAY_PIN_A, finalWaterStatus ? HIGH : LOW);
  digitalWrite(RELAY_PIN_C, finalWaterStatus ? HIGH : LOW);

  display.clearDisplay();
  display.setTextSize(1);

  String textA = "A: " + String(moistureA) + "%";
  display.setCursor(centerX(textA), 0);
  display.println(textA);

  String textC = "B: " + String(moistureC) + "%";
  display.setCursor(centerX(textC), 15);
  display.println(textC);

  String wifiMsg = (WiFi.status() == WL_CONNECTED) ? "WiFi OK" : "WiFi Lost!";
  display.setCursor(centerX(wifiMsg), 50);
  display.println(wifiMsg);

  display.display();

  delay(1000);
}
