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

// === MENU ===
void showMenu() {
  Serial.println("\n========== 📋 MENU ==========");
  Serial.println("E → Enroll RFID + Fingerprint");
  Serial.println("L → Login (RFID + Fingerprint)");
  Serial.println("I → List all enrolled users");
  Serial.println("D → Delete a user (by Finger ID)");
  Serial.println("R → Reset all EEPROM & Finger DB");
  Serial.println("==============================\n");
}

// === LOGIN ===
void loginUser() {
  long distance = getDistanceCM();
  Serial.print("[Ultrasonic] Distance = ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance >= 50) {
    Serial.println("💤 No user detected. Login cancelled.\n");
    return;
  }

  Serial.println("🧍 User detected. Waiting for RFID...");
  byte uid[4];
  unsigned long start = millis();
  while (!readRFID(uid)) {
    if (millis() - start > 10000) {
      Serial.println("❌ RFID timeout.\n");
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
      Serial.println("❌ Fingerprint timeout.\n");
      return;
    }
  }

  Serial.print("🔐 Fingerprint ID: ");
  Serial.println(fid);

  if (isUserMatch(uid, fid)) {
    Serial.println("✅ Access Granted.\n");
    logToSerialCSV(uid, fid, distance, true);
  } else {
    Serial.println("❌ Access Denied. Mismatch.\n");
    logToSerialCSV(uid, fid, distance, false);
  }

  delay(3000);
}

// === ENROLL ===
void enrollUser() {
  Serial.println("\n🆕 ENROLL Mode: Please scan RFID...");

  byte new_uid[4];
  while (!readRFID(new_uid));
  Serial.print("📇 Scanned UID: ");
  printUID(new_uid);

  int fid = getNextAvailableFingerprintID();
  if (fid == -1) {
    Serial.println("❌ No fingerprint slots available.\n");
    return;
  }

  Serial.print("👉 Enrolling Fingerprint ID ");
  Serial.println(fid);

  if (!enrollFingerprint(fid)) {
    Serial.println("❌ Enrollment failed.\n");
    return;
  }

  saveUserToEEPROM(new_uid, fid, fid - 1);
  Serial.println("✅ User enrolled and paired.\n");
}

// === LIST ===
void listAllUsers() {
  Serial.println("\n📋 Enrolled Users:");
  bool anyFound = false;
  for (int i = 0; i < MAX_USERS; i++) {
    int base = i * USER_SIZE;
    int fid = EEPROM.read(base + 4);
    if (fid == 0xFF || fid == 0x00) continue;

    anyFound = true;
    Serial.print("ID ");
    Serial.print(fid);
    Serial.print(" ↔ UID: ");
    for (int j = 0; j < 4; j++) {
      byte b = EEPROM.read(base + j);
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      if (j < 3) Serial.print(":");
    }
    Serial.println();
  }

  if (!anyFound) Serial.println("🚫 No users found.");
  Serial.println();
}

// === DELETE ===
void deleteUser() {
  Serial.println("\n🗑️ DELETE USER — Enter Fingerprint ID:");

  while (!Serial.available());
  int id = Serial.parseInt();

  if (id < 1 || id >= 128) {
    Serial.println("❌ Invalid ID.");
    return;
  }

  int base = (id - 1) * USER_SIZE;
  for (int i = 0; i < USER_SIZE; i++) {
    EEPROM.write(base + i, 0xFF);
  }

  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    Serial.print("✅ Deleted Fingerprint ID ");
    Serial.println(id);
  } else {
    Serial.println("⚠️ Could not delete from sensor memory.");
  }
}

// === RESET (with dump and confirm) ===
void resetAllUsers() {
  Serial.println("\n⚠️ EEPROM DUMP BEFORE RESET:");
  listAllUsers();

  Serial.println("⚠️ Type 'Y' and press Enter to confirm FULL RESET:");

  while (!Serial.available());
  String confirm = Serial.readStringUntil('\n');
  confirm.trim();

  if (confirm != "Y" && confirm != "y") {
    Serial.println("❌ Reset cancelled.\n");
    return;
  }

  for (int i = 0; i < MAX_USERS * USER_SIZE; i++) {
    EEPROM.write(i, 0xFF);
  }

  for (int id = 1; id < 128; id++) {
    finger.deleteModel(id);
  }

  Serial.println("✅ All data wiped: EEPROM + Fingerprint sensor.\n");
}

// === HELPERS ===
long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH) * 0.034 / 2;
}

bool readRFID(byte *uid_store) {
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;
  for (byte i = 0; i < 4; i++) uid_store[i] = mfrc522.uid.uidByte[i];
  mfrc522.PICC_HaltA();
  return true;
}

void printUID(byte *uid) {
  for (int i = 0; i < 4; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    if (i < 3) Serial.print(":");
  }
  Serial.println();
}

int getFingerprintID() {
  if (finger.getImage() != FINGERPRINT_OK) return -1;
  if (finger.image2Tz() != FINGERPRINT_OK) return -1;
  if (finger.fingerSearch() != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

bool enrollFingerprint(int id) {
  int p;
  Serial.println("👉 Place finger...");
  while ((p = finger.getImage()) != FINGERPRINT_OK);
  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;
  Serial.println("✋ Remove finger..."); delay(2000);
  Serial.println("👉 Place same finger again...");
  while ((p = finger.getImage()) != FINGERPRINT_OK);
  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;
  if (finger.createModel() != FINGERPRINT_OK) return false;
  if (finger.storeModel(id) != FINGERPRINT_OK) return false;
  return true;
}

int getNextAvailableFingerprintID() {
  for (int id = 1; id < 128; id++) {
    if (finger.loadModel(id) != FINGERPRINT_OK)
      return id;
  }
  return -1;
}

void saveUserToEEPROM(byte uid[4], int fid, int index) {
  int base = index * USER_SIZE;
  for (int i = 0; i < 4; i++) EEPROM.write(base + i, uid[i]);
  EEPROM.write(base + 4, fid);
}

bool isUserMatch(byte *uid, int fid) {
  byte stored_uid[4];
  int stored_fid;
  for (int i = 0; i < MAX_USERS; i++) {
    int base = i * USER_SIZE;
    stored_fid = EEPROM.read(base + 4);
    if (stored_fid == 0xFF) break;
    bool match = true;
    for (int j = 0; j < 4; j++) {
      stored_uid[j] = EEPROM.read(base + j);
      if (stored_uid[j] != uid[j]) match = false;
    }
    if (match && stored_fid == fid) return true;
  }
  return false;
}

void logToSerialCSV(byte* uid, int fid, long distance, bool success) {
  Serial.print("LOG,");
  for (int i = 0; i < 4; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    if (i < 3) Serial.print(":");
  }
  Serial.print(",");
  Serial.print(fid);
  Serial.print(",");
  Serial.print(distance);
  Serial.print(",");
  Serial.println(success ? "Access Granted" : "Access Denied");
}
