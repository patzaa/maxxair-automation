#include "maxxfan_output.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/switch/switch.h"
#include "Arduino.h"  // Add this for pinMode, digitalWrite, HIGH, LOW, etc.

namespace esphome {
namespace maxxfan {

static const char *TAG = "maxxfan";

// Define global variables
int gpio_up_pin = 0;
int gpio_down_pin = 0;
int gpio_power_pin = 0;
int gpio_direction_pin = 0;
int gpio_auto_pin = 0;
int actual_fan_speed = 4;
bool actual_fan_power = false;
bool actual_fan_direction = true;
bool actual_fan_cover = false;
bool is_boot_finished = false;

// Define external access to the switch entity for publishing state
namespace {
  switch_::Switch *maxxfan_cover = nullptr;
}

void SpeedOutput::setup() {
  pinMode(gpio_up_pin, OUTPUT);
  pinMode(gpio_down_pin, OUTPUT);
  ESP_LOGD(TAG, "SpeedOutput setup complete");
}

void SpeedOutput::loop() {
  if (!this->stepping_)
    return;

  uint32_t now = millis();
  if (now - this->last_step_time_ < 100)
    return;

  this->last_step_time_ = now;

  if (this->pin_high_) {
    // Release the pin after 100ms HIGH
    if (this->target_speed_ > actual_fan_speed) {
      digitalWrite(gpio_up_pin, LOW);
    } else {
      digitalWrite(gpio_down_pin, LOW);
    }
    this->pin_high_ = false;

    // Update speed tracker after completing one pulse
    if (this->target_speed_ > actual_fan_speed && actual_fan_speed < 10) {
      ++actual_fan_speed;
      ESP_LOGD(TAG, "Speed changed to: %d", actual_fan_speed);
    } else if (this->target_speed_ < actual_fan_speed && actual_fan_speed > 1) {
      --actual_fan_speed;
      ESP_LOGD(TAG, "Speed changed to: %d", actual_fan_speed);
    }

    // Check if we've reached the target
    if (actual_fan_speed == this->target_speed_) {
      this->stepping_ = false;
    }
  } else {
    // Drive the appropriate pin HIGH for next step
    if (this->target_speed_ > actual_fan_speed && actual_fan_speed < 10) {
      digitalWrite(gpio_up_pin, HIGH);
      this->pin_high_ = true;
    } else if (this->target_speed_ < actual_fan_speed && actual_fan_speed > 1) {
      digitalWrite(gpio_down_pin, HIGH);
      this->pin_high_ = true;
    } else {
      // Target reached or out of range
      this->stepping_ = false;
    }
  }
}

void SpeedOutput::write_state(float state) {
  int FanSet = state * 10;
  if (FanSet == 0 || FanSet == actual_fan_speed || actual_fan_power == 0 || !is_boot_finished)
    return;
  this->target_speed_ = FanSet;
  this->stepping_ = true;
  this->pin_high_ = false;
  this->last_step_time_ = millis() - 100;  // Start immediately
}

void PowerOutput::setup() {
  pinMode(gpio_power_pin, OUTPUT);
  ESP_LOGD(TAG, "PowerOutput setup complete");
}

void PowerOutput::write_state(bool FanSet) {
  // Don't do anything if still booting or if request already matches actual
  if (FanSet != actual_fan_power && is_boot_finished) {
    digitalWrite(gpio_power_pin, HIGH);
    delay(100);
    digitalWrite(gpio_power_pin, LOW);
    actual_fan_power = FanSet;
    actual_fan_cover = FanSet; // Maxxfan automatically changes cover on power action

    // Access the switch entity from the yaml configuration to publish state
    for (auto *sw : App.get_switches()) {
      if (sw->get_name() == "Cover") {
        maxxfan_cover = sw;
        break;
      }
    }

    if (maxxfan_cover != nullptr) {
      maxxfan_cover->publish_state(FanSet);  // Update front end
    }

    ESP_LOGD(TAG, "Power changed to: %s", actual_fan_power ? "On" : "Off");
    ESP_LOGD(TAG, "Cover changed to: %s", actual_fan_cover ? "Open" : "Close");
  }
}

void DirectionOutput::setup() {
  pinMode(gpio_direction_pin, OUTPUT);
  ESP_LOGD(TAG, "DirectionOutput setup complete");
}

void DirectionOutput::write_state(bool FanSet) {
  // Don't do anything if still booting, or if request already matches actual
  // or if power is turned off  
  if ((FanSet != actual_fan_direction) && (actual_fan_power != 0) && is_boot_finished) {
    digitalWrite(gpio_direction_pin, HIGH);
    delay(100);
    digitalWrite(gpio_direction_pin, LOW);
    actual_fan_direction = FanSet;
    ESP_LOGD(TAG, "Direction changed to: %s", actual_fan_direction ? "Reverse" : "Forward");
  }
}

void CoverOutput::setup() {
  pinMode(gpio_up_pin, OUTPUT);
  pinMode(gpio_down_pin, OUTPUT);
  
  // Find and store a reference to the maxxfan_cover switch entity
  for (auto *sw : App.get_switches()) {
    if (sw->get_name() == "Cover") {
      maxxfan_cover = sw;
      break;
    }
  }
  
  ESP_LOGD(TAG, "CoverOutput setup complete");
}

void CoverOutput::write_state(bool FanSet) {
  // Don't do anything if still booting or if request already matches actual 
  if (FanSet != actual_fan_cover && is_boot_finished) {
    // Need to simultaneously press the up and down buttons
    digitalWrite(gpio_up_pin, HIGH);
    digitalWrite(gpio_down_pin, HIGH);
    delay(200);
    digitalWrite(gpio_up_pin, LOW);
    digitalWrite(gpio_down_pin, LOW);
    actual_fan_cover = FanSet;
    ESP_LOGD(TAG, "Cover changed to: %s", actual_fan_cover ? "Open" : "Close");
  }
}

}  // namespace maxxfan
}  // namespace esphome