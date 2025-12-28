#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ze08_ch2o {

static const char *TAG = "ze08_ch2o";
static const uint8_t ZE08_SET_QA_MODE[] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};
static const uint8_t ZE08_QUESTION[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
static const char *ZE08_MODE_QA = "QA";
static const char *ZE08_MODE = ZE08_MODE_QA;

class ZE08CH2OSensor : public PollingComponent, public uart::UARTDevice {
 public:
  ZE08CH2OSensor() : uart::UARTDevice(nullptr) {}
  
  void set_uart_parent(uart::UARTComponent *parent) {
    this->parent_ = parent;
    this->uart::UARTDevice::set_uart_parent(parent);
  }
  
  void set_formaldehyde_sensor(sensor::Sensor *sensor) { this->ch2o_ppb_ = sensor; }
  
  void setup() override {
    this->set_update_interval(5000);
    if (ZE08_MODE == ZE08_MODE_QA) {
      write_array(ZE08_SET_QA_MODE, sizeof(ZE08_SET_QA_MODE));
    }
  }
  
  void update() override {
      // 1. 发送查询指令（如果你确定要用 QA 模式）
      write_array(ZE08_QUESTION, sizeof(ZE08_QUESTION));
      
      // 给传感器一点响应时间
      delay(50); 

      // 2. 循环检查缓冲区
      while (this->available() >= 9) {
          if (this->read() == 0xFF) { // 找到起始位
              unsigned char buf[9];
              buf[0] = 0xFF;
              if (this->read_array(&buf[1], 8)) {
                  // 3. 校验和检查
                  unsigned char checksum_calc = ~(buf[1] + buf[2] + buf[3] + buf[4] + buf[5] + buf[6] + buf[7]) + 1;
                  if (buf[8] != checksum_calc) {
                      ESP_LOGE(TAG, "Bad checksum! %02X != %02X", buf[8], checksum_calc);
                      continue; // 校验失败，继续找下一个 0xFF
                  }

                  // 4. 解析数据
                  unsigned short concentration_ppb = (buf[6] << 8) | buf[7];
                  ESP_LOGD(TAG, "Formaldehyde Concentration: %u ppb", concentration_ppb);

                  if (this->ch2o_ppb_ != nullptr) {
                      this->ch2o_ppb_->publish_state(concentration_ppb);
                  }
                  
                  // 读取成功后，建议清空当前缓冲区剩余的旧数据，防止干扰下次读取
                  while(this->available() > 0) this->read();
                  return; 
              }
          }
      }
      
      // 如果运行到这里，说明没有找到完整合法的数据包
      ESP_LOGW(TAG, "No valid packet found in UART buffer. Available: %d bytes", this->available());
  }

 protected:
  sensor::Sensor *ch2o_ppb_{nullptr};
  uart::UARTComponent *parent_{nullptr};
};

}  // namespace ze08_ch2o
}  // namespace esphome
