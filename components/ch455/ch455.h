#include "esphome.h"

using namespace esphome;

// 段码表 (0-9, '-', 空格)，位7是DP，位6-0是G-A 
static const uint8_t CH455_FONT[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, // 0-9
    0x40, 0x00 // '-' (index 10), ' ' (index 11)
};

static const uint8_t CH455_CMD_SYS = 0x48; 
static const uint8_t CH455_DIG0_ADDR = 0x68; 

class CH455Component : public Component {
 public:
  CH455Component(InternalGPIOPin *scl, InternalGPIOPin *sda) {
    scl_pin_ = scl;
    sda_pin_ = sda;
  }

  void setup() override {
    scl_pin_->setup();
    sda_pin_->setup();
    // 开启显示，8段模式 [cite: 163]
    send_command(CH455_CMD_SYS, 0x01); 
  }

  // 显示温度的具体逻辑
  void display_temperature(float temp) {
    uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};
    
    // 处理负数
    bool is_neg = temp < 0;
    if (is_neg) temp = -temp;

    // 取整数部分和小数一位 (例如 25.4)
    int val = (int)(temp * 10);
    
    // 填入数据 (假设从右往左显示)
    data[0] = CH455_FONT[val % 10];         // 小数点后第一位
    data[1] = CH455_FONT[(val / 10) % 10] | 0x80; // 个位 + 小数点 
    data[2] = CH455_FONT[(val / 100) % 10]; // 十位
    
    if (is_neg) {
        data[3] = CH455_FONT[10]; // 显示负号
    } else if (val >= 1000) {
        data[3] = CH455_FONT[(val / 1000) % 10]; // 百位
    } else {
        data[3] = CH455_FONT[11]; // 空格
    }

    // 写入芯片 
    for (uint8_t i = 0; i < 4; i++) {
      send_command(CH455_DIG0_ADDR + (i * 2), data[i]);
    }
  }

 protected:
  InternalGPIOPin *scl_pin_;
  InternalGPIOPin *sda_pin_;

  void send_command(uint8_t b1, uint8_t b2) {
    start(); write_byte(b1); write_byte(b2); stop();
  }

  void start() {
    sda_pin_->digital_write(true); scl_pin_->digital_write(true);
    delayMicroseconds(2); sda_pin_->digital_write(false);
    delayMicroseconds(2); scl_pin_->digital_write(false);
  }

  void stop() {
    sda_pin_->digital_write(false); scl_pin_->digital_write(true);
    delayMicroseconds(2); sda_pin_->digital_write(true);
    delayMicroseconds(2);
  }

  void write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
      sda_pin_->digital_write(data & 0x80);
      delayMicroseconds(2);
      scl_pin_->digital_write(true);
      delayMicroseconds(2);
      scl_pin_->digital_write(false);
      data <<= 1;
    }
    sda_pin_->digital_write(true); scl_pin_->digital_write(true);
    delayMicroseconds(2); scl_pin_->digital_write(false);
  }
};