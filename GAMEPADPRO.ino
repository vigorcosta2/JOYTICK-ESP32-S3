#include <Arduino.h>
#include <BleGamepad.h>
#include "USB.h"
#include "USBHID.h"
#include "USBHIDGamepad.h"
#include <FastLED.h>

// ===================== CONFIGURAÇÕES =====================
#define NUM_BUTTONS 16
#define DEVICE_NAME "VGPRO"
#define BATTERY_PIN 1        // ADC1_CH0 - leitura da bateria
#define R1 100000.0
#define R2 100000.0

#define LED_PIN 38           // Pino LEDs RGB
#define NUM_LEDS 4

#define CHARGING_PIN 39      // Entrada TP4056 CHRG
#define FULL_PIN 40          // Entrada TP4056 STDBY

#define UPDATE_INTERVAL 1000 // Atualização de bateria (ms)

// ===================== OBJETOS =====================
BleGamepad bleGamepad(DEVICE_NAME, "ProMaker", 100);
USBHIDGamepad usbGamepad;
CRGB leds[NUM_LEDS];

// ===================== PINOS DOS BOTÕES (SEGUROS) =====================
const uint8_t buttonPins[NUM_BUTTONS] = {
  2, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18
};

bool lastButtonState[NUM_BUTTONS] = {0};
float batteryVoltage = 0.0;
uint8_t batteryPercent = 0;
bool isCharging = false;
bool isFull = false;
unsigned long lastBatteryUpdate = 0;

// ===================== FUNÇÕES =====================

// Leitura da bateria via divisor resistivo
void readBatteryLevel() {
  int adcValue = analogRead(BATTERY_PIN);
  float v = (adcValue / 4095.0) * 3.3 * ((R1 + R2) / R2);
  batteryVoltage = v;
  batteryPercent = constrain(map(v * 100, 300, 420, 0, 100), 0, 100);

  bleGamepad.setBatteryLevel(batteryPercent);

  Serial.print("Tensão: ");
  Serial.print(batteryVoltage, 2);
  Serial.print(" V | Carga: ");
  Serial.print(batteryPercent);
  Serial.println("%");
}

// Leitura do status de carregamento via TP4056
void readChargingStatus() {
  isCharging = (digitalRead(CHARGING_PIN) == LOW); // CHRG LED aceso → LOW
  isFull = (digitalRead(FULL_PIN) == LOW);         // STDBY LED aceso → LOW

  if (isCharging) {
    Serial.println("🔌 Carregando...");
  } else if (isFull) {
    Serial.println("✅ Bateria cheia");
  }
}

// Atualiza LEDs conforme nível e status
void updateLeds() {
  FastLED.clear();

  if (isCharging) {
    // Pisca em amarelo durante carga
    static bool toggle = false;
    toggle = !toggle;
    for (int i = 0; i < NUM_LEDS; i++)
      leds[i] = toggle ? CRGB::Yellow : CRGB::Black;
  } 
  else if (isFull) {
    // Verde total quando carregada
    fill_solid(leds, NUM_LEDS, CRGB::Green);
  } 
  else {
    // Indica nível de carga (vermelho → verde)
    uint8_t numLit = map(batteryPercent, 0, 100, 0, NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < numLit)
        leds[i] = CRGB::Green;
      else
        leds[i] = CRGB::Red;
    }
  }

  FastLED.show();
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  // Botões
  for (int i = 0; i < NUM_BUTTONS; i++) pinMode(buttonPins[i], INPUT_PULLUP);

  // BLE + USB
  bleGamepad.begin();
  usbGamepad.begin();
  USB.begin();

  // LEDs
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  // ADC
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  // Pinos de status do carregador
  pinMode(CHARGING_PIN, INPUT_PULLUP);
  pinMode(FULL_PIN, INPUT_PULLUP);

  delay(200);
}

// ===================== LOOP =====================
void loop() {
  // Atualiza bateria e LEDs
  if (millis() - lastBatteryUpdate >= UPDATE_INTERVAL) {
    lastBatteryUpdate = millis();
    readBatteryLevel();
    readChargingStatus();
    updateLeds();
  }

  // BLE Gamepad
  if (bleGamepad.isConnected()) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
      bool pressed = !digitalRead(buttonPins[i]);
      if (pressed != lastButtonState[i]) {
        lastButtonState[i] = pressed;
        if (pressed) bleGamepad.press(i + 1);
        else bleGamepad.release(i + 1);
      }
    }
  }

  // USB Gamepad
  uint32_t buttons = 0;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (!digitalRead(buttonPins[i])) buttons |= (1UL << i);
  }
  usbGamepad.send(0, 0, 0, 0, 0, 0, 0, buttons);

  delay(50);
}
