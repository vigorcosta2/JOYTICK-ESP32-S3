#include <Arduino.h>
#include <BleGamepad.h>
#include "USB.h"
#include "USBHID.h"
#include "USBHIDGamepad.h"

#define NUM_BUTTONS 16
const char* DEVICE_NAME = "VGPRO";

// --- BLE Gamepad ---
BleGamepad bleGamepad(DEVICE_NAME, "ProMaker", 100);

// --- USB Gamepad (TinyUSB nativo) ---
USBHIDGamepad usbGamepad;

// --- PINOS (todos seguros no ESP32-S3) ---
const uint8_t buttonPins[NUM_BUTTONS] = {
  2, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18
};

// Estado anterior dos botões (para debounce BLE)
bool lastButtonState[NUM_BUTTONS] = {0};

void setup() {
  // Configura pinos como entrada com pull-up interno
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Inicializa BLE e USB
  bleGamepad.begin();
  usbGamepad.begin();
  USB.begin();

  delay(100); // pequena pausa para estabilização
}

void loop() {
  // --- BLE GAMEPAD ---
  if (bleGamepad.isConnected()) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
      bool pressed = !digitalRead(buttonPins[i]); // LOW = pressionado

      // Envia atualização apenas se houve mudança
      if (pressed != lastButtonState[i]) {
        lastButtonState[i] = pressed;
        if (pressed) {
          bleGamepad.press(i + 1);  // Botões 1–16
        } else {
          bleGamepad.release(i + 1);
        }
      }
    }
  }

  // --- USB GAMEPAD ---
  uint32_t buttons = 0;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (!digitalRead(buttonPins[i])) {
      buttons |= (1UL << i);
    }
  }

  // Envia estado de todos os botões via USB
  usbGamepad.send(0, 0, 0, 0, 0, 0, 0, buttons);

  delay(10); // estabilidade / debounce
}
