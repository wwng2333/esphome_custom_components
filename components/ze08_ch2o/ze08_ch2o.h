#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ze08_ch2o {

static const char *TAG = "ze08_ch2o";

class ZE08CH2OSensor : public PollingComponent, public uart::UARTDevice {
 public:
  // 构造函数
  ZE08CH2OSensor() : uart::UARTDevice(nullptr) {}

  // 绑定传感器对象
  void set_formaldehyde_sensor(sensor::Sensor *sensor) { this->ch2o_ppb_ = sensor; }

  void setup() override {
    ESP_LOGI(TAG, "Initializing ZE08-CH2O...");
    // 建议预热时间为 3-5 分钟 [cite: 40, 126]
    // 发送切换到问答模式的指令 [cite: 80]
    static const uint8_t ZE08_SET_QA_MODE[] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};
    this->write_array(ZE08_SET_QA_MODE, sizeof(ZE08_SET_QA_MODE));
  }

  void update() override {
    // 1. 清理 UART 缓冲区中的过期数据，防止读取到之前的堆积 [cite: 74]
    while (this->available() > 0) {
      this->read();
    }

    // 2. 发送读取浓度指令 [cite: 88, 91]
    static const uint8_t ZE08_QUESTION[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    this->write_array(ZE08_QUESTION, sizeof(ZE08_QUESTION));

    // 3. 等待传感器响应，延时约 100ms
    this->set_timeout(100, [this]() {
      if (this->available() < 9) {
        ESP_LOGW(TAG, "No response from ZE08-CH2O yet. Available: %d bytes", this->available());
        return;
      }

      unsigned char buf[9];
      if (this->read_array(buf, 9)) {
        // 4. 打印原始十六进制数据，方便调试数据状态
        ESP_LOGI(TAG, "Raw Data: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);

        // 5. 验证起始位 [cite: 77, 91]
        if (buf[0] != 0xFF) {
          ESP_LOGW(TAG, "Invalid start byte detected: %02X", buf[0]);
          return;
        }

        // 6. 校验和计算：取反(Byte1+Byte2+...+Byte7) + 1 
        unsigned char sum = 0;
        for (int i = 1; i < 8; i++) {
          sum += buf[i];
        }
        unsigned char checksum_calc = (~sum) + 1;

        if (buf[8] != checksum_calc) {
          ESP_LOGE(TAG, "Checksum mismatch! Calculated: %02X, Received: %02X", checksum_calc, buf[8]);
          return;
        }

        // 7. 解析浓度数据 [cite: 78, 95]
        // 问答模式响应中，ppb 高位在 buf[6]，低位在 buf[7] [cite: 93]
        // 主动上传模式中，ppb 高位在 buf[4]，低位在 buf[5] [cite: 77]
        unsigned short concentration_ppb = 0;

        if (buf[1] == 0x86) { // 问答模式响应头 [cite: 93]
          concentration_ppb = (buf[6] << 8) | buf[7];
          unsigned short concentration_ugm3 = (buf[2] << 8) | buf[3];
          ESP_LOGD(TAG, "Parsed (Q&A Mode): %u ppb | %u ug/m3", concentration_ppb, concentration_ugm3);
        } 
        else if (buf[1] == 0x17) { // 主动上传模式响应头 [cite: 77]
          concentration_ppb = (buf[4] << 8) | buf[5];
          ESP_LOGD(TAG, "Parsed (Active Mode): %u ppb", concentration_ppb);
        }

        // 8. 发布数据到 Home Assistant
        if (this->ch2o_ppb_ != nullptr) {
          this->ch2o_ppb_->publish_state(concentration_ppb);
        }

        // 如果读数锁定在 5000，提示量程上限 
        if (concentration_ppb >= 5000) {
          ESP_LOGW(TAG, "Sensor reached full range (5000 ppb / 5 ppm). Check for interference.");
        }
      }
    });
  }

 protected:
  sensor::Sensor *ch2o_ppb_{nullptr};
};

}  // namespace ze08_ch2o
}  // namespace esphome
