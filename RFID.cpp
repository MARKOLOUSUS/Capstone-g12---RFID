#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* -------------------- PIN DEFINITIONS -------------------- */
#define SS_PIN   D4
#define RST_PIN  D3
#define BUZZER   D8

/* -------------------- WIFI SETTINGS -------------------- */
const char* ssid = "ZTE_2.4G_k3uCHR";
const char* password = "UPpPMCDY";

/* -------------------- GOOGLE SCRIPT -------------------- */
const char* scriptURL =
  "https://script.google.com/macros/s/AKfycbxxl4SjAIOzKwYRJRpDpt_WodZtCztSkLHHDZ76-fBrG9VI0jHyCmsJpqi1kBTv3rkgUg/exec";

/* -------------------- OBJECTS -------------------- */
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* -------------------- GLOBAL -------------------- */
bool wifiConnected = false;

/* -------------------- SOUND FUNCTION -------------------- */
void happySound() {
 tone(BUZZER, 1500, 100);
delay(120);
tone(BUZZER, 2000, 150);
}

void wowSound() {
  tone(BUZZER, 500, 100);
  delay(100);
  tone(BUZZER, 1500, 100);
  delay(120);
  tone(BUZZER, 2000, 150);
}

/* -------------------- SETUP -------------------- */
void setup() {
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();
  pinMode(BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(100);
    Serial.print(".");
  }

  lcd.clear();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;

    lcd.print("WiFi OK");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());

    happySound();  
  } else {
    wifiConnected = false;

    lcd.print("WiFi FAILED");
    lcd.setCursor(0, 1);
    lcd.print("Offline Mode");

    tone(BUZZER, 300, 400); 
  }

  delay(1000);
  lcd.clear();
  lcd.print("Ready");
}

/* -------------------- LOOP -------------------- */
void loop() {

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  lcd.clear();
  lcd.print("UID:");
  lcd.setCursor(0, 1);
  lcd.print(uid);

  happySound();

  /* ---------- SEND ONLY IF WIFI OK ---------- */
  if (wifiConnected) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    String url = String(scriptURL) + "?uid=" + uid;
    http.begin(client, url);

    int httpCode = http.GET();
    String payload = http.getString();
    http.end();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(payload.substring(0, 16));
    lcd.setCursor(0, 1);
    if (payload.length() > 16) {
      lcd.print(payload.substring(16, 32));
    }

    wowSound();  
  } else {
    lcd.clear();
    lcd.print("No WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Offline");

    tone(BUZZER, 300, 400);  
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1000);
}
