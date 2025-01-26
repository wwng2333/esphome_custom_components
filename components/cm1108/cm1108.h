#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace cm1108 {

enum CM1108ABCLogic { CM1108_ABC_NONE = 0, CM1108_ABC_ENABLED, CM1108_ABC_DISABLED };

class CM1108Component : public PollingComponent, public uart::UARTDevice {
 public:
  float get_setup_priority() const override;

  void setup() override;
  void update() override;
  void dump_config() override;

  void calibrate_zero();
  void abc_enable();
  void abc_disable();

  void set_co2_sensor(sensor::Sensor *co2_sensor) { co2_sensor_ = co2_sensor; }
  void set_abc_enabled(bool abc_enabled) { abc_boot_logic_ = abc_enabled ? CM1108_ABC_ENABLED : CM1108_ABC_DISABLED; }
  void set_warmup_seconds(uint32_t seconds) { warmup_seconds_ = seconds; }

 protected:
  bool cm1108_write_command_(const uint8_t *command, uint8_t command_length, uint8_t *response, uint8_t response_length);

  sensor::Sensor *co2_sensor_{nullptr};
  CM1108ABCLogic abc_boot_logic_{CM1108_ABC_NONE};
  uint32_t warmup_seconds_;
};

template<typename... Ts> class CM1108CalibrateZeroAction : public Action<Ts...> {
 public:
  CM1108CalibrateZeroAction(CM1108Component *cm1108) : cm1108_(cm1108) {}

  void play(Ts... x) override { this->cm1108_->calibrate_zero(); }

 protected:
  CM1108Component *cm1108_;
};

template<typename... Ts> class CM1108ABCEnableAction : public Action<Ts...> {
 public:
  CM1108ABCEnableAction(CM1108Component *cm1108) : cm1108_(cm1108) {}

  void play(Ts... x) override { this->cm1108_->abc_enable(); }

 protected:
  CM1108Component *cm1108_;
};

template<typename... Ts> class CM1108ABCDisableAction : public Action<Ts...> {
 public:
  CM1108ABCDisableAction(CM1108Component *cm1108) : cm1108_(cm1108) {}

  void play(Ts... x) override { this->cm1108_->abc_disable(); }

 protected:
  CM1108Component *cm1108_;
};

}  // namespace cm1108
}  // namespace esphome