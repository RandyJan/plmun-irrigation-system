#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==== WiFi Credentials ====
const char* ssid = "YourWiFiName";       // <-- change this
const char* password = "YourWiFiPassword"; // <-- change this

// ==== OLED Setup ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== Moisture sensor pins ====
#define MOISTURE_PIN_A 34
#define MOISTURE_PIN_B 33
#define MOISTURE_PIN_C 32

// ==== Relay pins ====
#define RELAY_PIN_A 2   
#define RELAY_PIN_B 4    
#define RELAY_PIN_C 16   

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA=21, SCL=22 for ESP32

  // ==== OLED Init ====
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // ==== Relays Init ====
  pinMode(RELAY_PIN_A, OUTPUT);
  pinMode(RELAY_PIN_B, OUTPUT);
  pinMode(RELAY_PIN_C, OUTPUT);

  // Start with all pumps OFF
  digitalWrite(RELAY_PIN_A, LOW);
  digitalWrite(RELAY_PIN_B, LOW);
  digitalWrite(RELAY_PIN_C, LOW);

  // ==== WiFi Connection ====
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Show on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.setCursor(0, 15);
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
}

void loop() {
  // Read moisture sensors
  int sensorA = analogRead(MOISTURE_PIN_A);
  int sensorB = analogRead(MOISTURE_PIN_B);
  int sensorC = analogRead(MOISTURE_PIN_C);

  // Map values to percentage (adjust calibration if needed)
  int moistureA = map(sensorA, 4095, 1500, 0, 100);
  int moistureB = map(sensorB, 4095, 1500, 0, 100);
  int moistureC = map(sensorC, 4095, 1500, 0, 100);

  // Constrain between 0–100
  moistureA = constrain(moistureA, 0, 100);
  moistureB = constrain(moistureB, 0, 100);
  moistureC = constrain(moistureC, 0, 100);

  // Print to Serial Monitor
  Serial.print("A: "); Serial.print(moistureA); Serial.print("% | ");
  Serial.print("B: "); Serial.print(moistureB); Serial.print("% | ");
  Serial.print("C: "); Serial.print(moistureC); Serial.println("%");

  // Control relays (ON if < 40%)
  digitalWrite(RELAY_PIN_A, (moistureA < 40) ? HIGH : LOW);
  digitalWrite(RELAY_PIN_B, (moistureB < 40) ? HIGH : LOW);
  digitalWrite(RELAY_PIN_C, (moistureC < 40) ? HIGH : LOW);

  // Display on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("A: "); display.print(moistureA); display.println("%");

  display.setCursor(0, 15);
  display.print("B: "); display.print(moistureB); display.println("%");

  display.setCursor(0, 30);
  display.print("C: "); display.print(moistureC); display.println("%");

  // Show WiFi status
  display.setCursor(0, 50);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi OK");
  } else {
    display.print("WiFi Lost!");
  }

  display.display();

  delay(1000);
}
