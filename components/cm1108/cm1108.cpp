#include "cm1108.h"
#include "esphome/core/log.h"

#include <cinttypes>

namespace esphome {
namespace cm1108 {

//datasheet https://github.com/esphome/feature-requests/files/5839487/Single.Beam.NDIR.CO2.Sensor.Module.CM1106.Specification.pdf

static const char *const TAG = "cm1108";
static const uint8_t CM1108_PPM_RESPONSE_LENGTH = 8;
static const uint8_t CM1108_ABC_COMMAND_LENGTH = 9;
static const uint8_t CM1108_COMMAND_GET_PPM[] = {0x11, 0x01, 0x01};
static const uint8_t CM1108_COMMAND_ABC_ENABLE[] =  {0x11, 0x07, 0x10, 0x64, 0x00, 0x1E, 0x01, 0x90, 0x64};
static const uint8_t CM1108_COMMAND_ABC_DISABLE[] = {0x11, 0x07, 0x10, 0x64, 0x02, 0x07, 0x01, 0x90, 0x64};
static const uint8_t CM1108_COMMAND_CALIBRATE_ZERO[] = {0x11, 0x03, 0x03, 0x01, 0x90};

uint8_t cm1108_checksum(const uint8_t *command, uint8_t command_length) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < command_length; i++) {
    sum += command[i];
  }
  return 0x100 - sum % 0x100;
}

void CM1108Component::setup() {
  //ESP_LOGW(TAG, "Start up code.");
  if (this->abc_boot_logic_ == CM1108_ABC_ENABLED) {
    this->abc_enable();
  } else if (this->abc_boot_logic_ == CM1108_ABC_DISABLED) {
    this->abc_disable();
  }
}

void CM1108Component::update() {
  uint32_t now_ms = millis();
  uint32_t warmup_ms = this->warmup_seconds_ * 1000;
  if (now_ms < warmup_ms) {
    ESP_LOGW(TAG, "CM1108 warming up, %" PRIu32 " s left", (warmup_ms - now_ms) / 1000);
    this->status_set_warning();
    return;
  }

  uint8_t response[CM1108_PPM_RESPONSE_LENGTH];
  if (!this->cm1108_write_command_(CM1108_COMMAND_GET_PPM, 3, response, CM1108_PPM_RESPONSE_LENGTH)) {
    ESP_LOGW(TAG, "Reading data from CM1108 failed!");
    this->status_set_warning();
    return;
  }

  if (response[0] != 0x16 || response[1] != 0x05 || response[2] != 0x01) {
    ESP_LOGW(TAG, "Invalid preamble from CM1108!");
    this->status_set_warning();
    return;
  }

  uint8_t checksum = cm1108_checksum(response, CM1108_PPM_RESPONSE_LENGTH - 1);
  if (response[CM1108_PPM_RESPONSE_LENGTH - 1] != checksum) {
    ESP_LOGW(TAG, "CM1108 Checksum doesn't match: 0x%02X!=0x%02X", response[CM1108_PPM_RESPONSE_LENGTH - 1], checksum);
    this->status_set_warning();
    return;
  }

  this->status_clear_warning();
  const uint16_t ppm = (uint16_t(response[3]) << 8) | response[4];

  ESP_LOGD(TAG, "CM1108 Received CO₂=%uppm", ppm);
  if (this->co2_sensor_ != nullptr)
    this->co2_sensor_->publish_state(ppm);
}

void CM1108Component::calibrate_zero() {
  ESP_LOGD(TAG, "CM1108 Calibrating zero point");
  this->cm1108_write_command_(CM1108_COMMAND_CALIBRATE_ZERO, 5, nullptr, 0);
}

void CM1108Component::abc_enable() {
  ESP_LOGD(TAG, "CM1108 Enabling automatic baseline calibration");
  uint8_t response_buf[10];
  this->cm1108_write_command_(CM1108_COMMAND_ABC_ENABLE, CM1108_ABC_COMMAND_LENGTH, nullptr, 0);
}

void CM1108Component::abc_disable() {
  ESP_LOGD(TAG, "CM1108 Disabling automatic baseline calibration");
  this->cm1108_write_command_(CM1108_COMMAND_ABC_DISABLE, CM1108_ABC_COMMAND_LENGTH, nullptr, 0);
}

bool CM1108Component::cm1108_write_command_(const uint8_t *command, uint8_t command_length, uint8_t *response, uint8_t response_length) {
  // Empty RX Buffer
  while (this->available())
    this->read();
  this->write_array(command, command_length);
  this->write_byte(cm1108_checksum(command, command_length));
  this->flush();

  if (response == nullptr)
    return true;

  return this->read_array(response, response_length);
}

float CM1108Component::get_setup_priority() const { return setup_priority::DATA; }
void CM1108Component::dump_config() {
  ESP_LOGCONFIG(TAG, "CM1108:");
  LOG_SENSOR("  ", "CO2", this->co2_sensor_);
  this->check_uart_settings(9600);

  if (this->abc_boot_logic_ == CM1108_ABC_ENABLED) {
    ESP_LOGCONFIG(TAG, "  Automatic baseline calibration enabled on boot");
  } else if (this->abc_boot_logic_ == CM1108_ABC_DISABLED) {
    ESP_LOGCONFIG(TAG, "  Automatic baseline calibration disabled on boot");
  }

  ESP_LOGCONFIG(TAG, "  Warmup time: %" PRIu32 " s", this->warmup_seconds_);
}

}  // namespace cm1108
}  // namespace esphome