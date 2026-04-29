/*************************************************************
   BLYNK + LCD + ENERGY METER (ACS712)
*************************************************************/

#define BLYNK_TEMPLATE_ID "TMPL2x_iDmKgF"
#define BLYNK_TEMPLATE_NAME "IoT Smart Energy Meter"
#define BLYNK_AUTH_TOKEN "mvyi3XzvOh8b7Hi-363tdDeZptD-qkXO"



#include <SoftwareSerial.h>
#include <BlynkSimpleStream.h>
#include <LiquidCrystal.h>

// ================= COMMUNICATION =================
SoftwareSerial SwSerial(6, 10); // RX, TX 
#define BLYNK_PRINT SwSerial
// ================= LCD =================
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// ================= PINS =================
#define ACS712_PIN A0
#define BUZZER_PIN 9
#define BTN_RESET 8
#define BTN_MONTH 7

// ================= CONSTANTES =================
float VOLTAGE = 230.0;
float TARIF = 50.0;
unsigned long MOIS_MS = 60000; // test (1 min)

// ================= VARIABLES =================
float totalConsommation = 0;
float totalCout = 0;

unsigned long startTime;
bool alarmActive = false;

BlynkTimer timer;

// ================= MESURE COURANT =================
float mesurerCourant() {
  float somme = 0;
  int N = 100;

  for (int i = 0; i < N; i++) {
    int val = analogRead(ACS712_PIN);
    float tension = val * (5.0 / 1023.0);
    float courant = (tension - 2.5) / 0.1;

    somme += courant * courant;
    delay(1);
  }

  float Irms = sqrt(somme / N);
  return Irms;
}

// ================= FONCTION PRINCIPALE =================
void calculEtAffichage() {

  float courant = mesurerCourant();
  float puissance = courant * VOLTAGE;
  float energie = puissance / 1000.0;

  // intégration énergie (chaque seconde)
  totalConsommation += energie * (1.0 / 3600.0);
  totalCout = totalConsommation * TARIF;

  // ===== LCD =====
  lcd.setCursor(0, 0);
  lcd.print(totalCout, 1);
  lcd.print(" CFA ");
  lcd.print(courant, 2);
  lcd.print("A   ");

  lcd.setCursor(0, 1);
  lcd.print(puissance, 1);
  lcd.print("W ");
  lcd.print(totalConsommation, 2);
  lcd.print("kWh ");

  // ===== BLYNK =====
  Blynk.virtualWrite(V0, VOLTAGE);
  Blynk.virtualWrite(V1, courant);
  Blynk.virtualWrite(V2, puissance);
  Blynk.virtualWrite(V3, totalConsommation);
  Blynk.virtualWrite(V4, totalCout);

  // Debug
  SwSerial.print("I: "); SwSerial.print(courant);
  SwSerial.print(" P: "); SwSerial.print(puissance);
  SwSerial.print(" E: "); SwSerial.print(totalConsommation);
  SwSerial.print(" C: "); SwSerial.println(totalCout);
}

// ================= BOUTONS =================
void gererBoutons() {

  // RESET
  if (!digitalRead(BTN_RESET)) {
    tone(BUZZER_PIN, 200, 1000);

    totalConsommation = 0;
    totalCout = 0;
    startTime = millis();

    delay(300);
  }
}

// ================= SETUP =================
void setup() {

  lcd.begin(16, 2);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_MONTH, INPUT_PULLUP);

  SwSerial.begin(115200);

  // COMPIM
  Serial.begin(9600);
  Blynk.begin(Serial, BLYNK_AUTH_TOKEN);

  // Message LCD
  lcd.print(" ENERGY METER");
  lcd.setCursor(0, 1);
  lcd.print(" Initialisation");
  delay(1000);
  lcd.clear();

  startTime = millis();

  // Timer (1 seconde)
  timer.setInterval(1000L, calculEtAffichage);
}

// ================= LOOP =================
void loop() {

  Blynk.run();
  timer.run();

  gererBoutons();

  // ===== ALARME FIN MOIS =====
  if (millis() - startTime >= MOIS_MS) {

    tone(BUZZER_PIN, 600);

    while (!alarmActive) {
      if (digitalRead(BTN_MONTH) == LOW) {
        tone(BUZZER_PIN, 200, 500);
        alarmActive = true;
      }
    }

    totalConsommation = 0;
    totalCout = 0;
    startTime = millis();
    alarmActive = false;

    noTone(BUZZER_PIN);
    delay(300);
  }
}