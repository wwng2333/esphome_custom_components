#include "SY210.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sy210 {

static const char *const TAG = "sy210";

static const uint8_t SY210_RESPONSE_HEADER[] = {0x42, 0x4d};
static const uint8_t SY210_REQUEST[] = {0x42, 0x4d};

void SY210Component::setup() {
  // because this implementation is currently rx-only, there is nothing to setup
}

void SY210Component::dump_config() {
  ESP_LOGCONFIG(TAG, "SY210:");
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(9600);
}

void SY210Component::update() {
  ESP_LOGV(TAG, "sending measurement request");
  //this->write_array(SY210_REQUEST, sizeof(SY210_REQUEST));

}

void SY210Component::loop() {
  while (this->available() != 0) {
    this->read_byte(&this->data_[this->data_index_]);
    auto check = this->check_byte_();
    if (!check.has_value()) {
      // finished
      this->parse_data_();
      this->data_index_ = 0;
    } else if (!*check) {
      // wrong data
      ESP_LOGV(TAG, "Byte %i of received data frame is invalid.", this->data_index_);
      this->data_index_ = 0;
    } else {
      // next byte
      this->data_index_++;
    }
  }
}

float SY210Component::get_setup_priority() const { return setup_priority::DATA; }

uint16_t SY210Component::sy210_checksum_(const uint8_t *command_data, uint8_t length) const {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < length; i++) {
    sum += command_data[i];
  }
  return sum;
}

optional<bool> SY210Component::check_byte_() const {
  uint8_t index = this->data_index_;
  uint8_t byte = this->data_[index];

  // index 0..1 are the fixed header
  if (index < sizeof(SY210_RESPONSE_HEADER)) {
    return byte == SY210_RESPONSE_HEADER[index];
  }
  // 接收0..5个字节
  if (index < (sizeof(SY210_RESPONSE_HEADER) + 3))
    return true;

  // 共接收了5个字节，开始计算checksum
  if (index == (sizeof(SY210_RESPONSE_HEADER) + 3)) {
    uint16_t checksum = sy210_checksum_(this->data_, sizeof(SY210_RESPONSE_HEADER) + 3);
    // 与接收到的checksum做对比
    if (checksum != 0x100) {
      ESP_LOGW(TAG, "SY210 checksum is wrong: %04x, expected zero", checksum);
      return false;
    }
    return {};
  }

  return false;
}

void SY210Component::parse_data_() {
  const int pm_2_5_concentration = this->get_16_bit_uint_(6);

  if (this->pm_2_5_sensor_ != nullptr) {
    this->pm_2_5_sensor_->publish_state(pm_2_5_concentration);
  }
}

uint16_t SY210Component::get_16_bit_uint_(uint8_t start_index) const {
  return encode_uint16(this->data_[start_index], this->data_[start_index + 1]);
}

}  // namespace sy210
}  // namespace esphome