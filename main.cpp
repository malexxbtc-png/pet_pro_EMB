#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "functions.h"
constexpr unsigned long debounceDelay = 50UL;
int systemMode = 0; 

int buttonState = HIGH;             
int lastReading = HIGH;             
unsigned long lastDebounceTime = 0;  
unsigned long previousLDRMillis = 0; 

Adafruit_NeoPixel rgbLed(1, 48, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  rgbLed.begin();
  rgbLed.setBrightness(40);
  updateStatusLED();

  // Реле вимкнено за замовчуванням (HIGH для Active LOW)
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println("==========================================");
  Serial.println("Систему запущено! Режим: АВТОМАТИЧНИЙ");
  Serial.println("==========================================");
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Опитування кнопки (з антибрязкотом на millis)
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = currentMillis;
  }

  if (currentMillis - lastDebounceTime > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        systemMode = (systemMode + 1) % 3; // Перемикання: 0 -> 1 -> 2 -> 0
        updateStatusLED();

        Serial.print("\n>>> ЗМІНА РЕЖИМУ: ");
        if (systemMode == 0) Serial.println("0: АВТОМАТИЧНИЙ (Зелений RGB)");
        else if (systemMode == 1) Serial.println("1: РУЧНИЙ - ВИМКНЕНО (Червоний RGB)");
        else if (systemMode == 2) Serial.println("2: РУЧНИЙ - УВІМКНЕНО (Синій RGB)");
      }
    }
  }
  lastReading = reading;

  // 2. Опитування LDR, Оновлення шкали та Керування реле
  if (currentMillis - previousLDRMillis >= INTERVAL_LDR) {
    previousLDRMillis = currentMillis;

    int ldrValue = analogRead(LDR_PIN);
    int leds = updateLightLevelBar(ldrValue); // Шкала оновлюється завжди

    if (leds != -1) {
      if (systemMode == 0) {
        // АВТОМАТИЧНИЙ: 1 діод (темно) -> LOW (УВІМК), інакше HIGH (ВИМК)
        digitalWrite(RELAY_PIN, (leds == 1) ? LOW : HIGH);
      } 
      else if (systemMode == 1) {
        // РУЧНИЙ ВИМК
        digitalWrite(RELAY_PIN, HIGH);
      } 
      else if (systemMode == 2) {
        // РУЧНИЙ УВІМК
        digitalWrite(RELAY_PIN, LOW);
      }

      // Моніторинг станiв
      Serial.print("LDR: ");
      Serial.print(ldrValue);
      Serial.print(" | Шкала: ");
      Serial.print(leds);
      Serial.print("/4 | Режим: ");
      if (systemMode == 0) Serial.print("Авто");
      else if (systemMode == 1) Serial.print("Ручний-ВИМК");
      else Serial.print("Ручний-УВІМК");

      Serial.print(" | Реле: ");
      Serial.println(digitalRead(RELAY_PIN) == LOW ? "УВІМК" : "ВИМК");
    }
  }
}