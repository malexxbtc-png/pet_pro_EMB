#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

extern int systemMode;
extern Adafruit_NeoPixel rgbLed;

// Оновлення колірного статусу на RGB LED
inline void updateStatusLED() {
  if (systemMode == 0) {
    rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0));   // Зелений — АВТО
  } else if (systemMode == 1) {
    rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));   // Червоний — РУЧНИЙ ВИМК
  } else if (systemMode == 2) {
    rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255));   // Синій — РУЧНИЙ УВІМК
  }
  rgbLed.show();
}

// Оновлення індикації 4-х світлодіодів освітленості (працює постійно)
inline int updateLightLevelBar(int ldrValue) {
  if (ldrValue == 0) {
    Serial.println("[ПОМИЛКА] Значення АЦП = 0!");
    return -1;
  }

  int numLedsToLight = 1;

  if (ldrValue < THRESHOLD_1) {
    numLedsToLight = 1;
  } else if (ldrValue < THRESHOLD_2) {
    numLedsToLight = 2;
  } else if (ldrValue < THRESHOLD_3) {
    numLedsToLight = 3;
  } else {
    numLedsToLight = 4;
  }

  digitalWrite(LED1_PIN, numLedsToLight >= 1 ? HIGH : LOW);
  digitalWrite(LED2_PIN, numLedsToLight >= 2 ? HIGH : LOW);
  digitalWrite(LED3_PIN, numLedsToLight >= 3 ? HIGH : LOW);
  digitalWrite(LED4_PIN, numLedsToLight >= 4 ? HIGH : LOW);

  return numLedsToLight;
}