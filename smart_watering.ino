#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==== WiFi Credentials ====
const char* ssid = "wifime";       // WiFi name
const char* password = "12345678"; // WiFi pass

// ==== OLED Setup ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== Moisture sensor pins ====
#define MOISTURE_PIN_A 34
#define MOISTURE_PIN_C 32

// ==== Relay pins ====
#define RELAY_PIN_A 2
#define RELAY_PIN_C 16

// Function to center text
int centerX(String text, int textSize = 1) {
  int textWidth = text.length() * 6 * textSize; 
  return (128 - textWidth) / 2;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // ==== OLED Init ====
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  // ==== Relays Init ====
  pinMode(RELAY_PIN_A, OUTPUT);
  pinMode(RELAY_PIN_C, OUTPUT);

  digitalWrite(RELAY_PIN_A, LOW);
  digitalWrite(RELAY_PIN_C, LOW);

  // ==== WiFi Connect ====
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Show on OLED
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

void loop() {

  // ==== Read Sensors ====
  int sensorA = analogRead(MOISTURE_PIN_A);
  int sensorC = analogRead(MOISTURE_PIN_C);

  int moistureA = map(sensorA, 4095, 1500, 0, 100);
  int moistureC = map(sensorC, 4095, 1500, 0, 100);

  moistureA = constrain(moistureA, 0, 100);
  moistureC = constrain(moistureC, 0, 100);

  // Serial debug
  Serial.print("A: "); Serial.print(moistureA); Serial.print("% | ");
  Serial.print("C: "); Serial.print(moistureC); Serial.println("%");

  // ==== Relay Logic ====
  digitalWrite(RELAY_PIN_A, (moistureA < 20) ? HIGH : LOW);
  digitalWrite(RELAY_PIN_C, (moistureC < 20) ? HIGH : LOW);

  // ==== OLED Display ====
  display.clearDisplay();
  display.setTextSize(1);

  // Line A
  String textA = "A: " + String(moistureA) + "%";
  display.setCursor(centerX(textA), 0);
  display.println(textA);

  // Line C
  String textC = "B: " + String(moistureC) + "%";
  display.setCursor(centerX(textC), 15);
  display.println(textC);

  // WiFi status
  String wifiMsg = (WiFi.status() == WL_CONNECTED) ? "WiFi OK" : "WiFi Lost!";
  display.setCursor(centerX(wifiMsg), 50);
  display.println(wifiMsg);

  display.display();

  delay(1000);
}
