#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/globals/globals_component.h"

namespace esphome {
namespace maxxfan {

// GPIO pin assignments for the WT32-ETH01 custom PCB. These are static wiring
// facts, so they stay hardcoded here (see maxxfan_output.cpp).
extern int gpio_up_pin;
extern int gpio_down_pin;
extern int gpio_power_pin;
extern int gpio_direction_pin;
extern int gpio_auto_pin;

// Fan state is owned by the YAML `globals:` (persisted to flash and driven by the
// on_boot handshake). The component holds POINTERS to those globals so there is a
// single source of truth shared between the lambda and the button logic — instead
// of the private, disconnected copies this component used to keep.
extern globals::RestoringGlobalsComponent<int> *g_actual_fan_speed;
extern globals::RestoringGlobalsComponent<bool> *g_actual_fan_power;
extern globals::RestoringGlobalsComponent<bool> *g_actual_fan_direction;
extern globals::RestoringGlobalsComponent<bool> *g_actual_fan_cover;
extern globals::GlobalsComponent<bool> *g_is_boot_finished;

// Bind the pointers above to the YAML globals. Called once from codegen.
void link_state_globals(globals::RestoringGlobalsComponent<int> *speed,
                        globals::RestoringGlobalsComponent<bool> *power,
                        globals::RestoringGlobalsComponent<bool> *direction,
                        globals::RestoringGlobalsComponent<bool> *cover,
                        globals::GlobalsComponent<bool> *boot);

class SpeedOutput : public Component, public output::FloatOutput {
 public:
  void setup() override;
  void write_state(float state) override;
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
