#define BLYNK_PRINT Serial

// ================= BLYNK =================
#define BLYNK_TEMPLATE_ID "TMPL6FDfnVPlW"
#define BLYNK_TEMPLATE_NAME "SPIS THESIS"
#define BLYNK_AUTH_TOKEN "aVLJS6_COot9QYTsMxkz9dUrTpalOwGX"

// ================= INCLUDES =================
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TimeLib.h>

// ================= WIFI =================
char ssid[] = "wifime";
char pass[] = "12345678";

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= PINS =================
#define MOISTURE_PIN_A 34
#define MOISTURE_PIN_C 32
#define RELAY_PIN_A 2
#define RELAY_PIN_C 16

// ================= CONSTANTS =================
#define MOISTURE_THRESHOLD 30
#define MAX_WATER_DURATION 300   // Max watering seconds (5 minutes)

// ================= STATE VARIABLES =================
bool manualWater = false;
bool scheduleActive = false;
bool autoWatering = false;
bool aiMode = false;

int autoStart = 0;
int autoStop = 0;
int aiDecision = 0;  // 0 = NO WATER, 1 = WATER

unsigned long lastNotify = 0;
unsigned long lastAIRequest = 0;
unsigned long wateringStartTime = 0;
unsigned long dailyWaterUsage = 0;  // seconds

// ================= HELPERS =================
int centerX(String text, int textSize = 1) {
  int textWidth = text.length() * 6 * textSize;
  return (SCREEN_WIDTH - textWidth) / 2;
}

// ================= BLYNK CALLBACKS =================
BLYNK_CONNECTED() {
  Blynk.syncAll();
  Blynk.sendInternal("rtc_sync");
}

BLYNK_WRITE(InternalPinRTC) {
  time_t t = param.asLong();
  setTime(t);
}
BLYNK_WRITE(V3) { manualWater = param.asInt(); }  // Manual toggle
BLYNK_WRITE(V8) { aiMode = param.asInt(); }      // AI toggle
BLYNK_WRITE(V4) {  // Schedule
  TimeInputParam t(param);
  if (t.hasStartTime() && t.hasStopTime()) {
    autoStart = t.getStartHour() * 3600 + t.getStartMinute() * 60;
    autoStop  = t.getStopHour()  * 3600 + t.getStopMinute()  * 60;
    scheduleActive = true;
  } else {
    scheduleActive = false;
  }
}

// ================= AI MOCK FUNCTION =================
void requestAI(int mA, int mC) {
  // ----- MOCK AI ----- waters if either < 30%
  aiDecision = (mA < 30 || mC < 30) ? 1 : 0;
}

// ================= NOTIFICATIONS =================
void sendNotification(String msg) {
  if (millis() - lastNotify > 60000) {  // max 1/min
    Blynk.logEvent("SEND_NOTIFICATION", msg);
    lastNotify = millis();
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(RELAY_PIN_A, OUTPUT);
  pinMode(RELAY_PIN_C, OUTPUT);
  digitalWrite(RELAY_PIN_A, LOW);
  digitalWrite(RELAY_PIN_C, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting to Blynk...");
  display.display();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

// ================= LOOP =================
void loop() {
  Blynk.run();

  // --- Read moisture sensors ---
  int rawA = analogRead(MOISTURE_PIN_A);
  int rawC = analogRead(MOISTURE_PIN_C);

  int moistureA = constrain(map(rawA, 4095, 1500, 0, 100), 0, 100);
  int moistureC = constrain(map(rawC, 4095, 1500, 0, 100), 0, 100);

  Blynk.virtualWrite(V5, moistureA);
  Blynk.virtualWrite(V0, moistureC);

  // --- AI REQUEST (every 5s) ---
  if (aiMode && millis() - lastAIRequest > 5000) {
    requestAI(moistureA, moistureC);
    lastAIRequest = millis();
  }

  // --- Scheduled watering ---
  time_t nowSec = hour() * 3600 + minute() * 60 + second();
bool scheduledWater = false;

if (scheduleActive) {
  if (autoStart <= autoStop) {
    // Normal schedule
    scheduledWater = (nowSec >= autoStart && nowSec <= autoStop);
  } else {
    // Overnight schedule
    scheduledWater = (nowSec >= autoStart || nowSec <= autoStop);
  }
}

  // --- Auto watering based on threshold ---
  if (!manualWater && !scheduledWater && !aiMode) {
    autoWatering = (moistureA < MOISTURE_THRESHOLD || moistureC < MOISTURE_THRESHOLD);
  } else {
    autoWatering = false;
  }

  // --- Final watering decision ---
  bool watering = manualWater || scheduledWater || (aiMode && aiDecision == 1) || (!aiMode && autoWatering);

  // --- Overwatering protection ---
  if (watering && !scheduledWater) {
    if (wateringStartTime == 0) wateringStartTime = millis();
    else if ((millis() - wateringStartTime) / 1000 > MAX_WATER_DURATION) {
      watering = false;  // Stop pump
      sendNotification("Watering stopped: Overwatering protection");
    }
  } else {
    if (wateringStartTime > 0) {
      dailyWaterUsage += (millis() - wateringStartTime) / 1000;
      wateringStartTime = 0;
    }
  }

  // --- Activate relays ---
  digitalWrite(RELAY_PIN_A, watering ? HIGH : LOW);
  digitalWrite(RELAY_PIN_C, watering ? HIGH : LOW);

  // --- Serial Monitor ---
  Serial.print("Moisture A: "); Serial.print(moistureA);
  Serial.print(" | Moisture C: "); Serial.print(moistureC);
  Serial.print(" | AI: "); Serial.print(aiMode);
  Serial.print(" | AI Decision: "); Serial.print(aiDecision);
  Serial.print(" | Watering: "); Serial.print(watering);
  Serial.print(" | Daily Usage(s): "); Serial.println(dailyWaterUsage);

  // --- Blynk Widgets ---
  Blynk.virtualWrite(V7, aiDecision ? "AI: WATERING" : "AI: WATER OFF"); // Labeled value
  Blynk.virtualWrite(V9, watering ? 255 : 0);  // LED
  Blynk.virtualWrite(V10, "Usage:" + String(dailyWaterUsage));    // Daily usage counter
  Blynk.virtualWrite(V11, aiDecision ? "AI: WATERING" : "AI: WATER OFF");
  // --- OLED Display ---
  display.clearDisplay();
  display.setCursor(centerX("A: " + String(moistureA) + "%"), 0);
  display.println("A: " + String(moistureA) + "%");
  display.setCursor(centerX("B: " + String(moistureC) + "%"), 15);
  display.println("B: " + String(moistureC) + "%");

  String mode = manualWater ? "MODE: MANUAL" :
                scheduledWater ? "MODE: TIMER" :
                aiMode ? "MODE: AI" :
                autoWatering ? "MODE: AUTO" : "MODE: IDLE";

  display.setCursor(centerX(mode), 35);
  display.println(mode);
  display.setCursor(0, 50);
  display.println("Usage: " + String(dailyWaterUsage) + "s");
  display.display();

  delay(1000);
}
