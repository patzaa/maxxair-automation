#include "maxxfan_output.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/switch/switch.h"
#include "Arduino.h"  // pinMode, digitalWrite, HIGH, LOW, etc.
#include <cmath>      // lroundf

namespace esphome {
namespace maxxfan {

static const char *TAG = "maxxfan";

// GPIO pin assignments for WT32-ETH01 custom PCB
// U18->Power=GPIO15, U17->Speed+=GPIO14, U16->Speed-=GPIO12
// U15->Direction=GPIO17, U14->Auto=GPIO5
int gpio_up_pin = 14;
int gpio_down_pin = 12;
int gpio_power_pin = 15;
int gpio_direction_pin = 17;
int gpio_auto_pin = 5;

// Pointers to the shared YAML globals (single source of truth). Bound once via
// link_state_globals() during codegen-generated setup.
globals::RestoringGlobalsComponent<int> *g_actual_fan_speed = nullptr;
globals::RestoringGlobalsComponent<bool> *g_actual_fan_power = nullptr;
globals::RestoringGlobalsComponent<bool> *g_actual_fan_direction = nullptr;
globals::RestoringGlobalsComponent<bool> *g_actual_fan_cover = nullptr;
globals::GlobalsComponent<bool> *g_is_boot_finished = nullptr;

void link_state_globals(globals::RestoringGlobalsComponent<int> *speed,
                        globals::RestoringGlobalsComponent<bool> *power,
                        globals::RestoringGlobalsComponent<bool> *direction,
                        globals::RestoringGlobalsComponent<bool> *cover,
                        globals::GlobalsComponent<bool> *boot) {
  g_actual_fan_speed = speed;
  g_actual_fan_power = power;
  g_actual_fan_direction = direction;
  g_actual_fan_cover = cover;
  g_is_boot_finished = boot;
}

// Publish the Cover switch state to the front end. The MaxxFan moves its lid
// automatically when power is pressed, so the switch must follow.
static void publish_cover(bool state) {
  for (auto *sw : App.get_switches()) {
    if (sw->get_name() == "Cover") {
      sw->publish_state(state);
      break;
    }
  }
}

void SpeedOutput::setup() {
  pinMode(gpio_up_pin, OUTPUT);
  pinMode(gpio_down_pin, OUTPUT);
  ESP_LOGD(TAG, "SpeedOutput setup complete");
}

void SpeedOutput::write_state(float state) {
  // state is 0.0..1.0; convert to an integer speed 0..10. Use lroundf, not a
  // bare cast: 0.7f * 10 == 6.9999 would truncate to 6.
  int FanSet = (int) lroundf(state * 10);
  int &speed = g_actual_fan_speed->value();
  // Do nothing while booting, if already at target, if power is off, or if the
  // UI requested 0 (which means "off" and is handled by the power output).
  if (FanSet == 0 || FanSet == speed || g_actual_fan_power->value() == 0 ||
      !g_is_boot_finished->value())
    return;
  // The MaxxFan only changes speed one step per button press, so ramp by pulsing
  // up/down until the tracker reaches the target. Blocking is intentional and
  // matches the power/direction/cover outputs (max ~9 steps).
  while (FanSet > speed && speed < 10) {
    digitalWrite(gpio_up_pin, HIGH);
    delay(100);
    digitalWrite(gpio_up_pin, LOW);
    delay(100);
    ++speed;
    ESP_LOGD(TAG, "Speed changed to: %d", speed);
  }
  while (FanSet < speed && speed > 1) {
    digitalWrite(gpio_down_pin, HIGH);
    delay(100);
    digitalWrite(gpio_down_pin, LOW);
    delay(100);
    --speed;
    ESP_LOGD(TAG, "Speed changed to: %d", speed);
  }
}

void PowerOutput::setup() {
  pinMode(gpio_power_pin, OUTPUT);
  ESP_LOGD(TAG, "PowerOutput setup complete");
}

void PowerOutput::write_state(bool FanSet) {
  if (FanSet != g_actual_fan_power->value() && g_is_boot_finished->value()) {
    digitalWrite(gpio_power_pin, HIGH);
    delay(100);
    digitalWrite(gpio_power_pin, LOW);
    g_actual_fan_power->value() = FanSet;
    g_actual_fan_cover->value() = FanSet;  // MaxxFan auto-moves the cover with power
    publish_cover(FanSet);
    ESP_LOGD(TAG, "Power changed to: %s", FanSet ? "On" : "Off");
    ESP_LOGD(TAG, "Cover changed to: %s", FanSet ? "Open" : "Close");
  }
}

void DirectionOutput::setup() {
  pinMode(gpio_direction_pin, OUTPUT);
  ESP_LOGD(TAG, "DirectionOutput setup complete");
}

void DirectionOutput::write_state(bool FanSet) {
  // Direction can only be changed while the fan is running.
  if (FanSet != g_actual_fan_direction->value() && g_actual_fan_power->value() != 0 &&
      g_is_boot_finished->value()) {
    digitalWrite(gpio_direction_pin, HIGH);
    delay(100);
    digitalWrite(gpio_direction_pin, LOW);
    g_actual_fan_direction->value() = FanSet;
    ESP_LOGD(TAG, "Direction changed to: %s", FanSet ? "Reverse" : "Forward");
  }
}

void CoverOutput::setup() {
  pinMode(gpio_up_pin, OUTPUT);
  pinMode(gpio_down_pin, OUTPUT);
  ESP_LOGD(TAG, "CoverOutput setup complete");
}

void CoverOutput::write_state(bool FanSet) {
  if (FanSet != g_actual_fan_cover->value() && g_is_boot_finished->value()) {
    // Press UP and DOWN simultaneously to toggle the lid.
    digitalWrite(gpio_up_pin, HIGH);
    digitalWrite(gpio_down_pin, HIGH);
    delay(200);
    digitalWrite(gpio_up_pin, LOW);
    digitalWrite(gpio_down_pin, LOW);
    g_actual_fan_cover->value() = FanSet;
    ESP_LOGD(TAG, "Cover changed to: %s", FanSet ? "Open" : "Close");
  }
}

}  // namespace maxxfan
}  // namespace esphome
