#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// === RFID ===
#define RST_PIN 9
#define SS_PIN 10
MFRC522 mfrc522(SS_PIN, RST_PIN);

// === Fingerprint ===
SoftwareSerial fingerSerial(2, 3);
Adafruit_Fingerprint finger(&fingerSerial);

// === Ultrasonic ===
#define TRIG_PIN 7
#define ECHO_PIN 6

// === EEPROM
#define USER_SIZE 5
#define MAX_USERS 204

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  finger.begin(57600);

  if (!finger.verifyPassword()) {
    Serial.println("❌ Fingerprint sensor not found.");
    while (1);
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("📌 System Ready.");
  showMenu();
}

void loop() {
  static unsigned long lastUltrasonicLog = 0;

  if (millis() - lastUltrasonicLog >= 1000) {
    long d = getDistanceCM();
    Serial.print("[Ultrasonic] Distance = ");
    Serial.print(d);
    Serial.println(" cm");
    lastUltrasonicLog = millis();
  }

  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'E': case 'e': enrollUser(); break;
      case 'L': case 'l': loginUser(); break;
      case 'I': case 'i': listAllUsers(); break;
      case 'D': case 'd': deleteUser(); break;
      case 'R': case 'r': resetAllUsers(); break;
      default: Serial.println("❓ Invalid command."); break;
    }
    showMenu();
  }
}

void loginUser() {
  long distance = getDistanceCM();
  Serial.print("[Ultrasonic] Distance = ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 25) {
    Serial.println("💤 No user nearby. System remains off.
");
    return;
  }

  Serial.println("⚡ System powered ON by user presence.");
  Serial.println("🧍 User detected. Waiting for RFID...");

  byte uid[4];
  unsigned long start = millis();
  while (!readRFID(uid)) {
    if (millis() - start > 10000) {
      Serial.println("❌ RFID timeout.
");
      return;
    }
  }

  Serial.print("📇 RFID UID: ");
  printUID(uid);

  Serial.println("👉 Waiting for fingerprint...");
  int fid = -1;
  start = millis();
  while (fid == -1) {
    fid = getFingerprintID();
    if (millis() - start > 10000) {
      Serial.println("❌ Fingerprint timeout.
");
      return;
    }
  }

  Serial.print("🔐 Fingerprint ID: ");
  Serial.println(fid);

  if (isUserMatch(uid, fid)) {
    Serial.println("✅ Access Granted.
");
    logToSerialCSV(uid, fid, distance, true);
  } else {
    Serial.println("❌ Access Denied. Mismatch.
");
    logToSerialCSV(uid, fid, distance, false);
  }

  delay(3000);
}

// [Omitted: remaining helper functions are same as previous full version]
