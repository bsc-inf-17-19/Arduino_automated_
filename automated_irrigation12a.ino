#include <WiFi.h>
#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
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
int cropThreshold = -1;  

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // NTP server, offset in seconds, update interval in ms

// Function to fetch the crop threshold value from Firebase
bool fetchCropThreshold() {
  Serial.println("Fetching crop threshold from Firebase...");
  if (Firebase.getInt(firebaseData, "/selectedCrop/value")) {
    if (firebaseData.dataType() == "int") {
      cropThreshold = firebaseData.intData();
      Serial.print("Fetched crop threshold: ");
      Serial.println(cropThreshold);
      return true;
    } else {
      Serial.print("Unexpected data type: ");
      Serial.println(firebaseData.dataType());
    }
  } else {
    Serial.print("Error fetching crop threshold: ");
    Serial.println(firebaseData.errorReason());
  }
  return false;
}

void setup() {
  Serial.begin(19200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi");

  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Initialize Firebase
  Serial.println("Initializing Firebase...");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) {
    Serial.println("Firebase connection successful");
  } else {
    Serial.println("Failed to connect to Firebase");
  }

  // Configure the moisture sensor pin as an input
  pinMode(MOISTURE_SENSOR_PIN, INPUT);

  // Configure the relay pin as an output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); 

  // Fetch the crop threshold value from Firebase
  while (!fetchCropThreshold()) {
    Serial.println("Retrying to fetch crop threshold...");
    delay(2000);
  }

  // Initialize NTP Client
  timeClient.begin();
}

void loop() {
  // Update NTP client
  timeClient.update();

  // Read the value from the moisture sensor
  int sensorValue = analogRead(MOISTURE_SENSOR_PIN);
  int moisturePercentage = map(sensorValue, ADC_MIN, ADC_MAX, 100, 0);
  moisturePercentage = constrain(moisturePercentage, 0, 100);

  // Log moisture percentage for debugging
  Serial.print("Moisture Level: ");
  Serial.print(moisturePercentage);
  Serial.println("%");

  // Control the relay based on the moisture level and crop threshold
  if (moisturePercentage < cropThreshold) {
    digitalWrite(RELAY_PIN, LOW); 
    Serial.println("Relay ON: Irrigation system activated");
  } else {
    digitalWrite(RELAY_PIN, HIGH); 
    Serial.println("Relay OFF: Irrigation system deactivated");
  }

  // Get the current timestamp
  String timestamp = timeClient.getFormattedTime();
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
  }

  // Add a delay before the next reading
  delay(10000); 
}
