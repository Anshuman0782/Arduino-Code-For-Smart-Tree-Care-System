#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>
#include <time.h>

// ===== Firebase Config =====
FirebaseConfig config;
FirebaseAuth auth;

// ===== WiFi Credentials =====
const char* ssid = "Your Wi-Fi name";
const char* password = "Your Wi-Fi Password";

// ===== Firebase Database URL =====
#define FIREBASE_HOST "Your FireBase Host Link"
#define FIREBASE_AUTH "Your Firebase Auth Link"

// ===== Sensor Pins =====
#define DHTPIN D2
#define DHTTYPE DHT11
#define SOIL_PIN A0
#define PIR_PIN D5
#define LDR_PIN D6
#define RELAY_PIN D1

DHT dht(DHTPIN, DHTTYPE);
FirebaseData fbdo;

float safeFloat(float v) {
  return isnan(v) ? 0 : v;
}

void setup() {
  Serial.begin(115200);

  // Relay OFF by default
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // FIX: prevent floating and ghost motion
  pinMode(PIR_PIN, INPUT_PULLUP);

  pinMode(LDR_PIN, INPUT);

  dht.begin();

  // ===== WiFi =====
  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected!");

  // ===== Firebase Setup =====
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // ===== NTP TIME SETUP =====
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("⌛ Syncing Time...");

  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n⏱ Time Synced!");
}

void loop() {

  float humidity = safeFloat(dht.readHumidity());
  float temperature = safeFloat(dht.readTemperature());

  int soilValue = analogRead(SOIL_PIN);
  int moisture = map(soilValue, 1023, 0, 0, 100);

  // FIX: use inverted reading for pull-up
  int motion = !digitalRead(PIR_PIN);

  int light = digitalRead(LDR_PIN);

  int pump = (moisture < 20) ? 1 : 0;
  digitalWrite(RELAY_PIN, pump ? LOW : HIGH);

  // ===== GET TIMESTAMP =====
  time_t now = time(nullptr);
  long timestamp = (long)now;

  // ===== CREATE JSON =====
  FirebaseJson json;
  json.add("temperature", temperature);
  json.add("humidity", humidity);
  json.add("moisture", moisture);
  json.add("motion", motion);
  json.add("light", light);
  json.add("pump", pump);
  json.add("timestamp", timestamp);

  // ===== WRITE TO currentData =====
  Firebase.setJSON(fbdo, "/currentData", json);

  // ===== WRITE TO rawData/{timestamp} =====
  String rawPath = "/rawData/" + String(timestamp);
  Firebase.setJSON(fbdo, rawPath, json);

  Serial.print("Motion = ");
  Serial.println(motion);

  Serial.println("📡 Firebase Updated!");

  delay(5000);
}
