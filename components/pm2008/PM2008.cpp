#include "PM2008.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pm2008 {

static const char *const TAG = "pm2008";

static const uint8_t PM2008_RESPONSE_HEADER[] = {0x43, 0x4d};
static const uint8_t PM2008_REQUEST[] = {0x43, 0x4d};

void PM2008Component::setup() {
  // because this implementation is currently rx-only, there is nothing to setup
}

void PM2008Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PM2008:");
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(9600);
}

void PM2008Component::update() {
  ESP_LOGV(TAG, "sending measurement request");
  //this->write_array(PM2008_REQUEST, sizeof(PM2008_REQUEST));

}

void PM2008Component::loop() {
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

float PM2008Component::get_setup_priority() const { return setup_priority::DATA; }

uint16_t PM2008Component::pm2008_checksum_(const uint8_t *command_data, uint8_t length) const {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < length; i++) {
    sum += command_data[i];
  }
  return sum;
}

optional<bool> PM2008Component::check_byte_() const {
  uint8_t index = this->data_index_;
  uint8_t byte = this->data_[index];

  // index 0..1 are the fixed header
  if (index < sizeof(PM2008_RESPONSE_HEADER)) {
    return byte == PM2008_RESPONSE_HEADER[index];
  }
  // 接收0..30个字节
  if (index < (sizeof(PM2008_RESPONSE_HEADER) + 29))
    return true;

  // 共接收了31个字节，开始计算checksum
  if (index == (sizeof(PM2008_RESPONSE_HEADER) + 29)) {
    uint16_t checksum = pm2008_checksum_(this->data_, sizeof(PM2008_RESPONSE_HEADER) + 27);
    // 与接收到的checksum做对比
    if (checksum != (this->data_[30] << 8 | this->data_[31])) {
      ESP_LOGW(TAG, "PM2008 checksum is wrong: %04x, expected zero", checksum);
      return false;
    }
    return {};
  }

  return false;
}

void PM2008Component::parse_data_() {
  const int pm_2_5_concentration = this->get_16_bit_uint_(6);

  if (this->pm_2_5_sensor_ != nullptr) {
    this->pm_2_5_sensor_->publish_state(pm_2_5_concentration);
  }
}

uint16_t PM2008Component::get_16_bit_uint_(uint8_t start_index) const {
  return encode_uint16(this->data_[start_index], this->data_[start_index + 1]);
}

}  // namespace pm2008
}  // namespace esphome