# 💊 IoT-Based Medibox with Alarm and Environment Monitoring

This project presents the design and implementation of a smart **IoT-based Medibox** using the **ESP32 microcontroller**, developed to assist patients with timely medication reminders and environmental condition alerts. The system includes time-based alarms, real-time clock synchronization, temperature and humidity warnings, and a user-friendly OLED interface. It is ideal for elderly care, chronic illness management, and remote health monitoring.

---

## 📌 Key Features

- ⏰ Set, view, and delete up to **2 medication alarms**
- 🌐 **NTP-based time sync** for accurate real-time clock
- 📟 **OLED menu interface** for user interaction
- 🔔 **Alarm system** with buzzer and stop/snooze control
- 🌡️ **Temperature and humidity monitoring** using DHT sensor
- 🚨 Automatic alerts when environment exceeds safe conditions
- 🔘 Interactive controls using onboard push buttons

---

## 🧠 System Overview

### 🔧 Hardware Components

- **Microcontroller**: ESP32 Dev Board
- **Display**: 0.96” I2C OLED (SSD1306)
- **Buttons**: 3 tactile push buttons (menu/select/scroll)
- **Sensors**: DHT11 for temperature & humidity
- **Actuators**:
  - Buzzer for alarm & warnings
  - LED indicator for environmental alerts

---

## 🧪 Software Features

- **Menu Navigation System**
  - Set Time Zone
  - Set / View / Delete Alarm 1 and Alarm 2
- **Alarm Functionality**
  - Time-matched trigger
  - Manual stop or 5-minute snooze
- **Sensor Warnings**
  - Display warning on OLED if temp/humidity is out of range
  - Activate LED and buzzer alert
- **NTP Time Sync**
  - Fetches real-time clock from internet via Wi-Fi
  - Keeps time accurate even after resets

---

## 🛠️ Project Architecture

### ⚙ Firmware Details

- Language: C++ using Arduino framework
- Libraries Used:
  - `WiFi.h`, `NTPClient.h`, `WiFiUdp.h`
  - `Adafruit_SSD1306`, `Adafruit_GFX`
  - `DHT.h`, `EEPROM.h`, `TimeLib.h`

### 🧰 Prototype Design (Wokwi Simulator Compatible)

- Developed and tested in [Wokwi Simulator](https://wokwi.com/)
- Simulation includes:
  - OLED, buttons, buzzer, LED, DHT11 sensor
  - Configurable interaction for full menu experience

---

## 📈 Performance & Evaluation

| Functionality             | Notes                                               |
|---------------------------|-----------------------------------------------------|
| **Alarm Accuracy**        | Synchronized with NTP; tested within ±1s deviation |
| **Environmental Sensing** | Warnings reliably triggered beyond thresholds       |
| **User Interface**        | Clear OLED navigation with responsive button input |
| **Reliability**           | Stable operation in extended tests (5+ hrs runtime) |

---

## 📷 System Media

> 📎 _[Add wiring diagram, simulation screenshot, and real hardware photos here]_  
> 📽️ _[Optional: Add YouTube or demo video link]_

---

## 📂 File Structure

