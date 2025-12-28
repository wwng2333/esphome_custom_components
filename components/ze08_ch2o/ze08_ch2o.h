#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ze08_ch2o {

static const char *const TAG = "ze08_ch2o";

class ZE08CH2OSensor : public Component, public uart::UARTDevice {
 public:
  // 构造函数
  ZE08CH2OSensor() : uart::UARTDevice() {}

  void set_formaldehyde_sensor(sensor::Sensor *sensor) { this->ch2o_ppb_ = sensor; }

  void setup() override {
    ESP_LOGI(TAG, "Initializing ZE08-CH2O in Passive/Active Listening mode...");
    // 如果传感器已经处于自动上报模式，这里无需发送指令
  }

  void loop() override {
    // 参照 SY210 的循环读取写法
    while (this->available() > 0) {
      uint8_t byte;
      this->read_byte(&byte);
      this->data_[this->data_index_] = byte;

      auto check = this->check_byte_();

      if (!check.has_value()) {
        // 数据包接收完成并校验通过
        this->parse_data_();
        this->data_index_ = 0;
      } else if (!*check) {
        // 校验失败或数据头不对，重置索引
        this->data_index_ = 0;
      } else {
        // 继续接收下一个字节
        this->data_index_++;
      }
    }
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  sensor::Sensor *ch2o_ppb_{nullptr};
  uint8_t data_[9];
  uint8_t data_index_{0};

  // 字节合法性校验逻辑
  optional<bool> check_byte_() {
    uint8_t index = this->data_index_;
    uint8_t byte = this->data_[index];

    // 1. 验证起始位 (Byte 0 必须是 0xFF)
    if (index == 0) {
      return byte == 0xFF;
    }

    // 2. 验证命令位 (Byte 1 在主动上传模式下通常是 0x17)
    if (index == 1) {
      return byte == 0x17;
    }

    // 3. 接收中间数据 (Byte 2 到 7)
    if (index < 8) {
      return true;
    }

    // 4. 接收到最后一个字节 (Byte 8)，验证校验和
    if (index == 8) {
      uint8_t checksum_calc = ze08_checksum_(this->data_);
      if (byte != checksum_calc) {
        ESP_LOGW(TAG, "ZE08 Checksum mismatch! Calc: %02X, Recv: %02X", checksum_calc, byte);
        return false;
      }
      return {}; // 返回空表示完整包校验成功
    }

    return false;
  }

  // ZE08 专用校验算法：(取反(Byte1+Byte2+...+Byte7)) + 1
  uint8_t ze08_checksum_(const uint8_t *data) {
    uint8_t sum = 0;
    for (uint8_t i = 1; i < 8; i++) {
      sum += data[i];
    }
    return (~sum) + 1;
  }

  // 解析并发布数据
  void parse_data_() {
    // 主动上传模式下：ppb 高位在 Byte 4，低位在 Byte 5
    uint16_t concentration_ppb = (uint16_t(this->data_[4]) << 8) | this->data_[5];
    
    ESP_LOGD(TAG, "ZE08 Received ppb: %u", concentration_ppb);

    if (this->ch2o_ppb_ != nullptr) {
      this->ch2o_ppb_->publish_state(concentration_ppb);
    }

    if (concentration_ppb >= 5000) {
      ESP_LOGW(TAG, "ZE08 Sensor reached full range!");
    }
  }
};

}  // namespace ze08_ch2o
}  // namespace esphome
