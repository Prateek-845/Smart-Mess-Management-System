#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Preferences.h>

// WiFi credentials
const char* ssid = "prateek";
const char* password = "headshot";

// Server configuration (VMware IP)
const char* serverURL = "http://192.168.153.49:8080/api/data";

// Sensor pins
#define PIR_ENTRY_PIN 2
#define PIR_EXIT_PIN 3
#define TRIG_PIN 5
#define ECHO_PIN 18
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define BUZZER_PIN 6
#define LED_PIN 7

const float BIN_THRESHOLD = 10.0;
volatile int occupancy = 0;
volatile int totalVisitors = 0;
int dustbinFullCount = 0;
DHT dht(DHT_PIN, DHT_TYPE);

Preferences preferences;

// Debouncing variables
volatile unsigned long lastEntryTime = 0;
volatile unsigned long lastExitTime = 0;
const unsigned long DEBOUNCE_DELAY = 500;

// Bin state variables
bool binWasFull = false;
unsigned long buzzerStartTime = 0;
const unsigned long BUZZER_DURATION = 1000;

// Interrupt handlers
void IRAM_ATTR entryDetected() {
  if (millis() - lastEntryTime > DEBOUNCE_DELAY) {
    occupancy++;
    totalVisitors++;
    preferences.putUInt("total", totalVisitors);
    lastEntryTime = millis();
  }
}

void IRAM_ATTR exitDetected() {
  if (millis() - lastExitTime > DEBOUNCE_DELAY && occupancy > 0) {
    occupancy--;
    lastExitTime = millis();
  }
}

// Modified ultrasonic function with default value if disconnected
float readUltrasonic(bool &ultrasonicConnected) {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 58000); // 58ms timeout (~10m)
  
  if(duration <= 0) {
    Serial.println("Ultrasonic sensor disconnected! Using default value.");
    ultrasonicConnected = false;
    return 1.0; // Default value for distance (cm)
  }
  ultrasonicConnected = true;
  return duration * 0.0343 / 2; // Convert to cm
}

String calculateFanSpeed(float temperature, float humidity) {
  String fanSpeed = "Low";
  if (!isnan(temperature) && !isnan(humidity)) {
    if (temperature > 28 || humidity > 60) fanSpeed = "High";
    else if (temperature > 24 || humidity > 50) fanSpeed = "Medium";
  }
  return fanSpeed;
}

void handleBinState(bool binIsFull, bool ultrasonicConnected) {
  if (ultrasonicConnected) {
    if (binIsFull && !binWasFull) {
      dustbinFullCount++;
      tone(BUZZER_PIN, 1000);
      buzzerStartTime = millis();
    }
    if (buzzerStartTime != 0 && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
      noTone(BUZZER_PIN);
      buzzerStartTime = 0;
    }
    binWasFull = binIsFull;
  }
}

void setup() {
  preferences.begin("mess", false);
  preferences.clear(); 
  preferences.end();
  
  Serial.begin(9600);
  pinMode(PIR_ENTRY_PIN, INPUT);
  pinMode(PIR_EXIT_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  dht.begin();

  preferences.begin("mess", false);
  totalVisitors = preferences.getUInt("total", 0);

  attachInterrupt(digitalPinToInterrupt(PIR_ENTRY_PIN), entryDetected, RISING);
  attachInterrupt(digitalPinToInterrupt(PIR_EXIT_PIN), exitDetected, RISING);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
}

void loop() {
  bool ultrasonicConnected = true;
  float distance = readUltrasonic(ultrasonicConnected);

  // Use default full_count if ultrasonic is disconnected
  int fullCountToSend = ultrasonicConnected ? dustbinFullCount : 1;

  // Handle bin state only if sensor connected
  if (ultrasonicConnected) {
    bool binIsFull = (distance <= BIN_THRESHOLD);
    handleBinState(binIsFull, ultrasonicConnected);
  }

  // Always read other sensors
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  String fanSpeed = calculateFanSpeed(temperature, humidity);

  // LED shows occupancy regardless of ultrasonic status
  digitalWrite(LED_PIN, occupancy > 0 ? HIGH : LOW);

  // Serial output
  Serial.print("Occupancy: ");
  Serial.print(occupancy);
  Serial.print(" | Total: ");
  Serial.print(totalVisitors);
  Serial.print(" | Distance: ");
  Serial.print(distance, 1);
  Serial.print("cm | Full Count: ");
  Serial.print(fullCountToSend);
  Serial.print(" | Temp: ");
  Serial.print(temperature, 1);
  Serial.print("°C | Humidity: ");
  Serial.print(humidity, 1);
  Serial.print("% | Fan: ");
  Serial.println(fanSpeed);

  // HTTP POST (always sends valid data)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{";
    jsonPayload += "\"occupancy\":" + String(occupancy);
    jsonPayload += ",\"total_visitors\":" + String(totalVisitors);
    jsonPayload += ",\"temperature\":" + String(temperature,1);
    jsonPayload += ",\"humidity\":" + String(humidity,1);
    jsonPayload += ",\"distance\":" + String(distance,1);
    jsonPayload += ",\"full_count\":" + String(fullCountToSend);
    jsonPayload += ",\"fan_speed\":\"" + fanSpeed + "\"}";

    int httpCode = http.POST(jsonPayload);
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Data sent successfully");
    } else {
      Serial.print("Error sending data, HTTP code: ");
      Serial.println(httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected - reconnecting");
    WiFi.reconnect();
  }

  delay(2000);
}
