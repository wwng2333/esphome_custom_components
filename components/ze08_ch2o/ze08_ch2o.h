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
     // 1. 清空缓冲区旧数据，确保拿到的是最新响应 [cite: 74]
     while (this->available() > 0) {
         this->read(); 
     }
 
     // 2. 发送查询指令 [cite: 91]
     static const uint8_t ZE08_QUESTION[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
     write_array(ZE08_QUESTION, sizeof(ZE08_QUESTION));
     
     delay(100); // 等待传感器响应
 
     // 3. 检查并读取数据 [cite: 71, 77]
     if (this->available() >= 9) {
         unsigned char buf[9];
         if (this->read_array(buf, 9)) {
             
             // --- 新增：打印原始数据 ---
             ESP_LOGI(TAG, "Raw Data: %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
                      buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
             // -----------------------
 
             // 4. 验证起始位 [cite: 77, 93]
             if (buf[0] != 0xFF) {
                 ESP_LOGW(TAG, "Invalid start byte: %02X", buf[0]);
                 return;
             }
 
             // 5. 校验和计算：取反(Byte1+Byte2+...+Byte7) + 1 [cite: 98, 109]
             unsigned char sum = 0;
             for(int i=1; i<8; i++) sum += buf[i];
             unsigned char checksum_calc = (~sum) + 1;
 
             if (buf[8] != checksum_calc) {
                 ESP_LOGE(TAG, "Checksum mismatch! Calc: %02X, Recv: %02X", checksum_calc, buf[8]);
                 return;
             }
 
             // 6. 解析浓度数据 [cite: 93, 95]
             // ug/m3: High byte (buf[2]) * 256 + Low byte (buf[3])
             unsigned short concentration_ugm3 = (buf[2] << 8) | buf[3]; 
             // ppb: High byte (buf[6]) * 256 + Low byte (buf[7])
             unsigned short concentration_ppb  = (buf[6] << 8) | buf[7]; 
             
             ESP_LOGD(TAG, "Parsed: %u ppb (%u ug/m3)", concentration_ppb, concentration_ugm3);
 
             if (this->ch2o_ppb_ != nullptr) {
                 this->ch2o_ppb_->publish_state(concentration_ppb);
             }
         }
     } else {
         ESP_LOGW(TAG, "No enough data. Available: %d bytes", this->available());
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

