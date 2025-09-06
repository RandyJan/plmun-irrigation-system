#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==== WiFi Credentials ====
const char* ssid = "SystemWifi";       // <-- change this
const char* password = "SystemWifi23"; // <-- change this

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

  // Constrain between 0–10
