#include "LED_Manager.h"
#include "define.h" // Include define.h for state definitions

// Variables for managing LED blinking.
// unsigned long lastBlink = 0; // No longer needed for fading states
// bool blinkState = false; // No longer needed for fading states

// Fading variables
uint8_t currentFadeValue = 0;
int8_t fadeDirection = 1; // 1 for fading up, -1 for fading down
unsigned long lastFadeUpdate = 0;
const unsigned long fadeIntervalMs = 10; // Update fade value every 10ms for smoothness

// Initializes the LED pins for PWM control.
void ledInit() {
  // Configure PWM channels for Red, Green, and Blue LEDs.
  // ledcSetup(channel, frequency, resolution_bits)
  ledcSetup(PWM_CH_R, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_G, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
  
  // Attach PWM channels to their respective GPIO pins.
  // ledcAttachPin(pin, channel)
  ledcAttachPin(PIN_R, PWM_CH_R);
  ledcAttachPin(PIN_G, PWM_CH_G);
  ledcAttachPin(PIN_B, PWM_CH_B);
  
  // Set initial LED color to off (0,0,0).
  ledSetColor(0,0,0);
}

// Sets the RGB color of the LED.
// r, g, b: 8-bit values (0-255) for Red, Green, and Blue intensity.
void ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
  // Write the analog value (PWM duty cycle) to each LED channel.
  // The resolution is 8 bits, so values range from 0 to 255.
  ledcWrite(PWM_CH_R, r);
  ledcWrite(PWM_CH_G, g);
  ledcWrite(PWM_CH_B, b);
}

// Helper to reset fade state when switching to a new fading pattern
void resetFadeState() {
  currentFadeValue = 0;
  fadeDirection = 1;
  lastFadeUpdate = millis();
}

// Controls LED blinking patterns based on the current device state.
// state: The current operational state of the device (from define.h).
// now: The current time in milliseconds (from millis()).
void ledBlink(int state, unsigned long now) {
  static int lastState = -1; // Track the previous state to detect changes

  // Respect Safe Mode: Keep LEDs off
  if (isSafeMode) return;

  // If state changed, reset fade for a fresh start
  if (state != lastState) {
    resetFadeState();
    lastState = state;
  }

  // Default 1Hz fade step: 255 / (500ms / 10ms) = 5
  uint8_t fadeStep = 5; 

  // Requirement: Fast fading for NTP & RTC Task (approx 3Hz)
  if (state == STATE_NTP_SYNC) fadeStep = 15;

  // Update fade value for states that use fading
  bool isFadingState = (state == STATE_NTP_SYNC || state == STATE_CONNECTING || 
                        state == STATE_ERROR || state == STATE_OTA_CHECK || 
                        state == STATE_OTA_UPDATE);

  if (isFadingState && (now - lastFadeUpdate >= fadeIntervalMs)) {
    lastFadeUpdate = now;
    currentFadeValue += fadeDirection * fadeStep;

    if (currentFadeValue >= 255) {
      currentFadeValue = 255;
      fadeDirection = -1; // Start fading down
    } else if (currentFadeValue <= 0) {
      currentFadeValue = 0;
      fadeDirection = 1; // Start fading up
    }
  }

  switch(state) {
    case STATE_NTP_SYNC: {
      ledSetColor(0, currentFadeValue, 0); // Green fade
      break;
    }
    case STATE_AP:
      ledSetColor(255, 255, 0);  // Solid yellow
      break;
    case STATE_CONNECTING: {
      ledSetColor(0, 0, currentFadeValue); // Blue fade
      break;
    }
    case STATE_CONNECTED:
      ledSetColor(0, 255, 0);  // Solid green
      break;
    case STATE_ERROR: {
      ledSetColor(currentFadeValue, 0, 0); // Red fade
      break;
    }
    case STATE_OTA_CHECK:
      ledSetColor(currentFadeValue, 0, currentFadeValue); // Magenta fade
      break;
    case STATE_OTA_UPDATE:
      ledSetColor(currentFadeValue, currentFadeValue, currentFadeValue); // White fade
      break;
    default:
      ledSetColor(0, 0, 0); // Off
      break;
  }
}
