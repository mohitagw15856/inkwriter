// InkWriter firmware entry point (Xteink X4/X3, ESP32-C3).
// TODO(hardware-test): whole boot path; see docs/HARDWARE_TESTING.md.
#ifdef ARDUINO

#include <Arduino.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <Logging.h>

#include <inkkit/inkkit.h>

#include "device/InkWriterApp.h"
#include "device/TextRenderer.h"

namespace {

inkkit::Display g_display(display);
inkkit::Buttons g_buttons(gpio);
inkwriter::TextRenderer g_tr(g_display);
inkwriter::InkWriterApp g_app(g_tr, g_buttons);

}  // namespace

void setup() {
  Serial.begin(115200);
  LOG_INF("IW", "InkWriter starting");

  gpio.begin();
  Storage.begin();
  g_display.begin();
  halClock.begin();
  powerManager.begin();

  g_app.begin();
}

void loop() {
  g_app.tick();

  if (g_buttons.powerHeldMs() >= 2000) {
    g_app.onBeforeSleep();
    inkkit::Power(powerManager, display, gpio).deepSleep();
  }
  delay(10);  // tighter loop than the readers: keystroke latency matters
}

#endif  // ARDUINO
