#  Smart Attendance System using RFID + Fingerprint + Ultrasonic

A complete end-to-end attendance system that combines RFID cards, fingerprint scanning, and ultrasonic presence detection. The system logs access events locally and visually using a real-time Streamlit dashboard, storing all data in a CSV file.

---

##  Features

-  RFID Authentication (via MFRC522)
-  Fingerprint Matching (via R307 module)
-  Ultrasonic Sensing (system powers ON only when user is within 25 cm)
-  EEPROM Storage of enrolled UID ↔ Fingerprint pairs
-  Serial Logging:
  - UID
  - Fingerprint ID
  - Distance from ultrasonic
  - Access status (Granted/Denied)
-  Streamlit Dashboard:
  - Live ultrasonic feed
  - Access event log
  - Downloadable CSV
-  Menu-driven Serial Interface:
  - `E` → Enroll (pair UID + Finger ID)
  - `L` → Login
  - `I` → List all users
  - `D` → Delete a user (by Fingerprint ID)
  - `R` → Reset (with EEPROM dump and confirmation)

---

##  How It Works

###  Login Flow:

1. Ultrasonic sensor detects user presence **within 25 cm**
2. RFID card is scanned
3. Fingerprint is scanned
4. If the UID ↔ Fingerprint ID match → ✅ Access Granted
5. Otherwise → ❌ Access Denied

All events are logged to Serial and CSV.

---

##  Hardware Required

| Component             | Description                     |
|----------------------|---------------------------------|
| Arduino Uno/Nano     | Main controller                 |
| MFRC522              | RFID card reader (SPI)          |
| R307                 | Fingerprint sensor (UART)       |
| HC-SR04              | Ultrasonic sensor               |
| LEDs/Buzzer (Optional) | For access status feedback    |

---

##  Wiring Overview

| Module       | Arduino Pins            |
|--------------|--------------------------|
| MFRC522      | SDA → D10, SCK → D13, MOSI → D11, MISO → D12, RST → D9 |
| R307         | TX → D2, RX → D3 (SoftwareSerial) |
| Ultrasonic   | Trig → D7, Echo → D6     |

Make sure to power R307 via 5V external supply if needed.

---

##  Setup & Run

### 1️⃣ Flash Arduino Code

- Upload `rfid_fingerprint_attendance.ino` to your Arduino board
- Open Serial Monitor at 9600 baud
- Use menu commands like `E`, `L`, `I`, `D`, `R`

### 2️⃣ Run Python Streamlit Dashboard

Install dependencies:
```bash
pip install streamlit pandas pyserial
```

Start the app:
```bash
streamlit run app.py
```

---

##  Project Structure

```
Smart_Attendance_System/
├── rfid_fingerprint_attendance.ino   ← Arduino logic
├── app.py                            ← Streamlit logger app
├── attendance_logger.py              ← CLI-based logger (optional)
├── attendance_log.csv                ← Auto-generated logs
├── .gitignore
└── README.md                         ← This file
```

---

##  Sample Output

###  Serial Monitor:
```
[Ultrasonic] Distance = 21 cm
⚡ System powered ON by user presence.
📇 RFID UID: D1:81:C0:01
🔐 Fingerprint ID: 5
✅ Access Granted

LOG,D1:81:C0:01,5,21,Access Granted
```

###  CSV Log:
```
Timestamp,UID,Fingerprint ID,Distance (cm),Access
2025-04-16 18:05:22,D1:81:C0:01,5,21,Access Granted
```

---

##  Reset & Safety

- `R` command prints EEPROM dump and asks for confirmation
- Type `'Y'` to proceed — this clears EEPROM and fingerprint DB

---

##  Future Add-ons

- SD card backup logging
- Wi-Fi or GSM cloud sync
- OLED display UI
- Mobile notification via Twilio or Telegram
- Admin override mode

---

##  Author

Made by Surya  
Powered by Arduino + Python + Streamlit

---

##  License

This project is open source under the MIT License.
