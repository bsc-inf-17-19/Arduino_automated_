#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Define the pin where the moisture sensor is connected
#define MOISTURE_SENSOR_PIN 34

// Define the range of ADC values for dry and wet soil
#define ADC_MIN 0        // ADC value for dry soil (adjust according to your sensor)
#define ADC_MAX 4095     // ADC value for wet soil (adjust according to your sensor)

// Replace with your network credentials
const char* ssid = "iope";
const char* password = "12345678";

// Replace with your Firebase project credentials
#define FIREBASE_HOST "automated-irrigation-sys-4d93e-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyDOVwK152RR46LB3H5zh6jbqmsG___dh2o"

// Create a Firebase Data object
FirebaseData firebaseData;

// Create FirebaseConfig and FirebaseAuth objects
FirebaseConfig config;
FirebaseAuth auth;

void setup() {
  // Initialize serial communication
  Serial.begin(19200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to Wi-Fi. IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Configure the moisture sensor pin as an input
  pinMode(MOISTURE_SENSOR_PIN, INPUT);

  // Initialize time (required for timestamp)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  // Read the value from the moisture sensor
  int sensorValue = analogRead(MOISTURE_SENSOR_PIN);

  // Map the sensor value to a percentage (0 to 100%)
  int moisturePercentage = map(sensorValue, ADC_MIN, ADC_MAX, 100, 0);

  // Constrain the percentage to be within 0 to 100
  moisturePercentage = constrain(moisturePercentage, 0, 100);

  // Print the moisture percentage to the serial monitor
  Serial.print("Moisture Level: ");
  Serial.print(moisturePercentage);
  Serial.println("%");

  // Get the current timestamp
  time_t now = time(nullptr);
  struct tm* p_tm = gmtime(&now);
  char buffer[20];
  strftime(buffer, 20, "%Y-%m-%dT%H:%M:%SZ", p_tm);
  String timestamp = String(buffer);

  // Create a JSON object with the sensor data
  FirebaseJson json;
  json.set("/value", moisturePercentage);
  json.set("/timestamp", timestamp);

  // Send data to Firebase
  String path = "/sensors/sensorId1/data/" + String(millis());
  if (Firebase.setJSON(firebaseData, path.c_str(), json)) {
    Serial.println("Data sent to Firebase successfully");
  } else {
    Serial.print("Error sending data: ");
    Serial.println(firebaseData.errorReason());
  }

  // Add a delay before the next reading
  delay(10000); // Delay increased to 10 seconds for less frequent updates
}
