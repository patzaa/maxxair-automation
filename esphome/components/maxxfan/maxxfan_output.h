#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace maxxfan {

// Define global variables
extern int gpio_up_pin;
extern int gpio_down_pin;
extern int gpio_power_pin;
extern int gpio_direction_pin;
extern int gpio_auto_pin;
extern int actual_fan_speed;
extern bool actual_fan_power;
extern bool actual_fan_direction;
extern bool actual_fan_cover;
extern bool is_boot_finished;

class SpeedOutput : public Component, public output::FloatOutput {
 public:
  void setup() override;
  void loop() override;
  void write_state(float state) override;

 protected:
  int target_speed_{0};
  bool stepping_{false};
  bool pin_high_{false};
  uint32_t last_step_time_{0};
};

class PowerOutput : public Component, public output::BinaryOutput {
 public:
  void setup() override;
  void write_state(bool state) override;
};

class DirectionOutput : public Component, public output::BinaryOutput {
 public:
  void setup() override;
  void write_state(bool state) override;
};

class CoverOutput : public Component, public output::BinaryOutput {
 public:
  void setup() override;
  void write_state(bool state) override;
};

}  // namespace maxxfan
}  // namespace esphome