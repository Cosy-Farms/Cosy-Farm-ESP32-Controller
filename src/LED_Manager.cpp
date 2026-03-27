#include "LED_Manager.h"

#define PIN_R 2
#define PIN_G 1
#define PIN_B 3

#define PWM_FREQ 1000
#define PWM_RES 8
#define PWM_CH_R 0
#define PWM_CH_G 1
#define PWM_CH_B 2

unsigned long lastBlink = 0;
bool blinkState = false;

void ledInit() {
  ledcSetup(PWM_CH_R, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_G, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_R, PWM_CH_R);
  ledcAttachPin(PIN_G, PWM_CH_G);
  ledcAttachPin(PIN_B, PWM_CH_B);
  ledSetColor(0,0,0);
}

void ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(PWM_CH_R, r);
  ledcWrite(PWM_CH_G, g);
  ledcWrite(PWM_CH_B, b);
}

void ledBlink(int state, unsigned long now) {
  switch(state) {
    case STATE_NTP_SYNC:
      if (now - lastBlink > 150) {
        blinkState = !blinkState;
        lastBlink = now;
        if (blinkState) {
          ledSetColor(100, 0, 255); // Magenta bright
        } else {
          ledSetColor(50, 0, 100); // Magenta dim
        }
      }
      break;
    case STATE_AP:
      ledSetColor(255, 255, 0);  // Solid yellow
      break;
    case STATE_CONNECTING:
      if (now - lastBlink > 1000) {
        blinkState = !blinkState;
        lastBlink = now;
        if (blinkState) {
          ledSetColor(0, 0, 255); // Blue bright
        } else {
          ledSetColor(0, 0, 50); // Blue dim
        }
      }
      break;
    case STATE_CONNECTED:
      ledSetColor(0, 255, 0);  // Solid green
      break;
    case STATE_ERROR:
      if (now - lastBlink > 200) {
        blinkState = !blinkState;
        lastBlink = now;
        if (blinkState) {
          ledSetColor(255, 0, 0); // Red bright
        } else {
          ledSetColor(100, 0, 0); // Red dim
        }
      }
      break;
    case STATE_OTA_CHECK:
      if (now - lastBlink > 1000) {
        blinkState = !blinkState;
        lastBlink = now;
        if (blinkState) {
          ledSetColor(0, 255, 255); // Cyan bright
        } else {
          ledSetColor(0, 100, 100); // Cyan dim
        }
      }
      break;
    case STATE_OTA_UPDATE:
      if (now - lastBlink > 200) {
        blinkState = !blinkState;
        lastBlink = now;
        if (blinkState) {
          ledSetColor(255, 255, 0); // Yellow bright
        } else {
          ledSetColor(100, 100, 0); // Yellow dim
        }
      }
      break;
    default:
      ledSetColor(0, 0, 0); // Off
      break;
  }
}

