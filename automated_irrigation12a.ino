#include <WiFi.h>
#include <HTTPClient.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>

// Define the pins
#define MOISTURE_SENSOR_PIN 34
#define RELAY_PIN 21

// Define the range of ADC values for dry and wet soil
#define ADC_MIN 0        
#define ADC_MAX 4095     

// Network credentials
const char* ssid = "iope";
const char* password = "12345678";

// Firebase project credentials
#define FIREBASE_HOST "automated-irrigation-sys-4d93e-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyDOVwK152RR46LB3H5zh6jbqmsG___dh2o"

// Create a Firebase Data object
FirebaseData firebaseData;

// Create FirebaseConfig and FirebaseAuth objects
FirebaseConfig config;
FirebaseAuth auth;

// Variable to store the crop threshold value
int cropThreshold;

// Function to fetch the current time from the World Time API
String getCurrentTime() {
  if (WiFi.status() == WL_CONNECTED) { // Check if the device is connected to Wi-Fi
    HTTPClient http;
    http.begin("http://worldtimeapi.org/api/timezone/Etc/UTC"); // Specify the URL
    int httpCode = http.GET(); // Make the GET request

    if (httpCode > 0) { // Check for the returning code
      String payload = http.getString();
      http.end();

      // Parse JSON response
      StaticJsonDocument<1024> jsonBuffer;
      deserializeJson(jsonBuffer, payload);
      const char* datetime = jsonBuffer["utc_datetime"];
      return String(datetime);
    } else {
      http.end();
      Serial.println("Error on HTTP request");
      return "";
    }
  } else {
    Serial.println("WiFi not connected");
    return "";
  }
}

// Function to fetch the crop threshold value from Firebase
bool fetchCropThreshold() {
  if (Firebase.getInt(firebaseData, "/selectedCrop/value")) {
    if (firebaseData.dataType() == "int") {
      cropThreshold = firebaseData.intData();
      Serial.print("Crop threshold value: ");
      Serial.println(cropThreshold);
      return true;
    } else {
      Serial.println("Failed to get integer data type from Firebase");
      return false;
    }
  } else {
    Serial.print("Error getting crop threshold value: ");
    Serial.println(firebaseData.errorReason());
    return false;
  }
}

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

  // Configure the relay pin as an output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure the relay is off initially

  // Fetch the crop threshold value from Firebase
  while (!fetchCropThreshold()) {
    Serial.println("Retrying to fetch crop threshold...");
    delay(2000);
  }
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

  // Control the relay based on the moisture level and crop threshold
  if (moisturePercentage < cropThreshold) {
    digitalWrite(RELAY_PIN, HIGH); // Turn on the relay
    Serial.println("Relay ON: Irrigation system activated");
  } else {
    digitalWrite(RELAY_PIN, LOW); // Turn off the relay
    Serial.println("Relay OFF: Irrigation system deactivated");
  }

  // Get the current timestamp from the World Time API
  String timestamp = getCurrentTime();

  // Log the timestamp for debugging
  Serial.print("Timestamp: ");
  Serial.println(timestamp);

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
    Serial.print("HTTP response code: ");
    Serial.println(firebaseData.httpCode());
  }

  // Add a delay before the next reading
  delay(10000); // Delay increased to 10 seconds for less frequent updates
}
