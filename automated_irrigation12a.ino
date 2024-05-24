#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> // Include ArduinoJson library

// Pin definitions for the ESP32
const int moistureSensorPin = 34; // Analog pin for the moisture sensor

// Replace with your network credentials
const char* ssid = "iope";
const char* password = "123456789/";

// Initialize the I2C LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16x2 display

// Initialize the web server
AsyncWebServer server(80);

// Variable to store the moisture percentage
int moisturePercentage = 0;

void setup() {
  Wire.begin(21, 22); // Initialize I2C communication with GPIO 21 (SDA) and GPIO 22 (SCL)
  Serial.begin(9600); // Initialize serial communication
  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  lcd.setCursor(0, 0);
  lcd.print("Moisture Sensor:");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Route to display moisture data
  server.on("/api/moisture", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    doc["moisture"] = moisturePercentage;
    String jsonString;
    serializeJson(doc, jsonString);
    request->send(200, "application/json", jsonString); // Moved the closing parenthesis and semicolon here
  });

  // Start server
  server.begin();
}

void loop() {
  // Read moisture value
  int moistureValue = analogRead(moistureSensorPin);

  // Map the moisture value to a percentage (assuming a full range of 0-4095 for ESP32)
  moisturePercentage = map(moistureValue, 4095, 0, 0, 100);

  // Display the moisture percentage on the LCD
  lcd.setCursor(0, 1);
  lcd.print("Moisture: ");
  lcd.print(moisturePercentage);
  lcd.print("%");

  // Print the moisture percentage to the Serial Monitor
  Serial.print("Moisture Percentage: ");
  Serial.print(moisturePercentage);
  Serial.println("%");

  // Delay before next reading
  delay(1000);
}
