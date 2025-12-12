/***************************************************************
  SMART IOT DOG FEEDER (ESP32)
  --------------------------------------------------------------
  Features:
   - RFID-based pet identification (MFRC522)
   - Daily feeding limit per UID (anti-overfeeding logic)
   - Environment validation using DHT22 (temp & humidity)
   - Servo-controlled food dispenser
   - Telegram alerts (WiFiClientSecure + Bot API)
   - Blynk remote control support
   - Scan tracking using std::map for per-day limits
***************************************************************/

#define BLYNK_TEMPLATE_ID ""          // REMOVE credentials
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <map>
#include <UniversalTelegramBot.h>

// -------------------- WiFi & Telegram --------------------
char ssid[] = "";       // <-- REMOVE CREDENTIALS
char pass[] = "";       // <-- REMOVE CREDENTIALS

#define BOT_TOKEN ""    // <-- REMOVE TOKEN
#define CHAT_ID ""      // <-- REMOVE CHAT ID

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// -------------------- Servo --------------------
Servo myServo;
int servoPin = 17;

// -------------------- RFID --------------------
#define SS_PIN 5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);

// -------------------- DHT22 --------------------
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Environmental thresholds
const float TEMP_THRESHOLD = 30.0;  
const float HUM_THRESHOLD = 80.0;

// -------------------- Blynk --------------------
bool blynkButtonState = false;
unsigned long sensorPrevMillis = 0;
const long sensorInterval = 2000;

unsigned long dhtPrevMillis = 0;
const long dhtTelegramInterval = 600000; // 10 mins

// -------------------- UID Daily Scan Tracking --------------------
struct CardInfo {
  int scanCount;
  unsigned long lastScanTime;
};

std::map<String, CardInfo> cardDatabase;

// Authorized RFID UIDs (ADD YOUR OWN UIDs)
String authorizedUIDs[] = {
  "736E9404",
  "739A3ADA",
  "241C2A02",
  "B5916331"
};

// -------------------- Utility Functions --------------------
void sendTelegram(String msg) {
  if (WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(CHAT_ID, msg, "");
  }
}

// -------------------- Environment Check --------------------
bool environmentOK() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  return (!isnan(t) && !isnan(h) &&
          t <= TEMP_THRESHOLD &&
          h <= HUM_THRESHOLD);
}

// -------------------- RFID Daily Limit Logic --------------------
bool isCardAllowed(String uid) {
  unsigned long now = millis();
  unsigned long oneDay = 24UL * 60UL * 60UL * 1000UL;

  CardInfo &info = cardDatabase[uid];

  // Reset count daily
  if (now - info.lastScanTime >= oneDay) {
    info.scanCount = 0;
    info.lastScanTime = now;
  }

  if (info.scanCount >= 3) {
    return false;
  }

  info.scanCount++;
  info.lastScanTime = now;
  return true;
}

// -------------------- Servo Movement --------------------
void moveServo() {
  myServo.write(90);
  Serial.println("Servo: Dispensing food...");
  sendTelegram("🍖 Food dispensed.");
  delay(3000);
  myServo.write(0);
  Serial.println("Servo: Returned to closed position.");
  delay(1000);
}

// -------------------- Read DHT Sensor --------------------
void readAndSendDHTData(bool sendToTelegramFlag) {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(t) || isnan(h)) {
    Serial.println("ERROR: Failed to read from DHT22.");
    if (sendToTelegramFlag) sendTelegram("⚠️ Failed to read DHT22 sensor!");
    return;
  }

  Serial.printf("Temp: %.2f°C | Humidity: %.2f%%\n", t, h);

  String msg = "🌡 Temp: " + String(t) + "°C | 💧 Humidity: " + String(h) + "%";

  if (sendToTelegramFlag) sendTelegram(msg);

  Blynk.virtualWrite(V4, t);
  Blynk.virtualWrite(V6, h);

  if (t > TEMP_THRESHOLD) Blynk.logEvent("high_temperature", "Temperature too high!");
  if (h > HUM_THRESHOLD) Blynk.logEvent("high_humidity", "Humidity too high!");
}

// -------------------- BLYNK Button Handler --------------------
BLYNK_WRITE(V0) {
  blynkButtonState = param.asInt();
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // WiFi
  WiFi.begin(ssid, pass);
  client.setInsecure();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi.");

  // Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  // Servo
  myServo.attach(servoPin);
  myServo.write(0);

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID ready...");

  // DHT
  dht.begin();
}

// -------------------- Main Loop --------------------
void loop() {
  Blynk.run();
  
  unsigned long currentMillis = millis();

  // Periodic DHT22 updates
  if (currentMillis - sensorPrevMillis >= sensorInterval) {
    sensorPrevMillis = currentMillis;
    readAndSendDHTData(false);
  }

  // Send periodic updates to Telegram
  if (currentMillis - dhtPrevMillis >= dhtTelegramInterval) {
    dhtPrevMillis = currentMillis;
    readAndSendDHTData(true);
  }

  // Check for RFID card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

    String scannedUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      scannedUID += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      scannedUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    scannedUID.toUpperCase();

    Serial.println("Card UID: " + scannedUID);
    sendTelegram("RFID Scan: " + scannedUID);

    bool authorized = false;
    for (String uid : authorizedUIDs) {
      if (scannedUID == uid) {
        authorized = true;
        break;
      }
    }

    if (authorized) {
      if (!environmentOK()) {
        Serial.println("ENV UNSAFE: Feeding blocked.");
        sendTelegram("🚫 Feeding blocked (environment unsafe).");
      } else if (!isCardAllowed(scannedUID)) {
        Serial.println("DAILY LIMIT REACHED.");
        sendTelegram("⚠️ Feeding blocked: Daily limit reached.");
      } else {
        Serial.println("AUTHORIZED. DISPENSING FOOD.");
        sendTelegram("✅ Authorized: Dispensing food.");
        moveServo();
      }
    } else {
      Serial.println("UNAUTHORIZED CARD!");
      sendTelegram("❌ Unauthorized card detected!");
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }

  // Handle Blynk remote trigger
  if (blynkButtonState) {
    if (environmentOK()) {
      moveServo();
    } else {
      Serial.println("ENV UNSAFE: Blynk flap blocked.");
      sendTelegram("🚫 Blynk: Feeding blocked due to unsafe environment.");
    }
  }
}
