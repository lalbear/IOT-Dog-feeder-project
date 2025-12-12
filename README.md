# Smart IoT Dog Feeder 🐶🍖  
An ESP32-based automated dog feeder with **RFID authentication**, **daily feeding limits**, **environment safety checks**, **servo-based food dispensing**, **Blynk control**, and **Telegram notifications**.

---

## 🚀 Features

### ✅ **RFID-based Pet Identification**
- Uses MFRC522 RFID module.
- Only registered UIDs can dispense food.
- Unauthorized scans trigger Telegram alerts.

### ✅ **Safety: Daily Feeding Limit**
- Each pet (UID) is allowed max **3 feeds per day**.
- Prevents accidental overfeeding.

### ✅ **Environment Monitoring (DHT22)**
- Reads **temperature & humidity** every 2 seconds.
- Blocks feeding if:
  - Temperature > 30°C  
  - Humidity > 80%  
- Sends warnings to Telegram & Blynk.

### ✅ **Servo-Controlled Feeder**
- Dispenses food by rotating servo to 90°, then closing back.

### ✅ **Cloud Integration**
- **Blynk**: Manual trigger, live sensor updates, event logs.
- **Telegram Bot**: Instant notifications for:
  - Authorized / unauthorized scans  
  - Overfeeding attempt  
  - Environment unsafe  
  - Feeds dispensed  
  - Periodic sensor updates  

---

## 🧰 Hardware Requirements

| Component | Purpose |
|----------|---------|
| ESP32 Dev Board | Main controller |
| MFRC522 RFID Reader | Pet identification |
| Servo Motor (SG90 / MG995) | Food dispenser flap |
| DHT22 Temperature & Humidity Sensor | Food spoilage detection |
| Breadboard + Jumper wires | Connections |
| External 5V supply (optional) | Stable servo power |

---

## 🛠 Wiring Diagram

### **ESP32 → MFRC522**
| MFRC522 | ESP32 |
|--------|--------|
| SDA | GPIO 5 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| 3.3V | 3.3V |
| GND | GND |

### **ESP32 → DHT22**
| DHT22 | ESP32 |
|-------|--------|
| VCC | 3.3V |
| DATA | GPIO 4 |
| GND | GND |

### **ESP32 → Servo**
| Servo | ESP32 |
|--------|--------|
| Signal | GPIO 17 |
| VCC | 5V |
| GND | GND |

---

## 📦 Software Libraries Required

Install via Arduino Library Manager:

- **MFRC522**
- **DHT sensor library**
- **Blynk**
- **UniversalTelegramBot**
- **ESP32Servo**

---

## 🔧 Configuration

Before uploading, fill in credentials:

```cpp
char ssid[] = "YOUR_WIFI";
char pass[] = "YOUR_PASS";

#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

String authorizedUIDs[] = { "UID1", "UID2", ... };
