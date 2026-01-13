#include "esphome.h"

using namespace esphome;

// CH455 命令与地址定义 [cite: 163, 173]
static const uint8_t CH455_CMD_SYS   = 0x48; 
static const uint8_t CH455_DIG0_ADDR = 0x68; 

class CH455Component : public PollingComponent, public display::DisplayBuffer {
 public:
  CH455Component(InternalGPIOPin *scl, InternalGPIOPin *sda) : PollingComponent(1000) {
    scl_pin_ = scl;
    sda_pin_ = sda;
  }

  void setup() override {
    scl_pin_->setup();
    sda_pin_->setup();
    // 初始化：8段模式, 开启显示, 最大亮度 [cite: 163, 166]
    send_command(CH455_CMD_SYS, 0x01); 
  }

  void update() override {
    this->display(); 
  }

  void display() {
    // 动态驱动4位数码管，地址从 68H 到 6EH [cite: 99, 173]
    for (uint8_t i = 0; i < 4; i++) {
      send_command(CH455_DIG0_ADDR + (i * 2), buffer_[i]);
    }
  }

  // 实现 DisplayBuffer 必需的虚函数
  void draw_absolute_pixel_internal(int x, int y, Color color) override {}
  int get_width_internal() override { return 4; }
  int get_height_internal() override { return 1; }

 protected:
  InternalGPIOPin *scl_pin_;
  InternalGPIOPin *sda_pin_;
  uint8_t buffer_[4] = {0};

  // 严格按照手册的通讯协议实现 
  void send_command(uint8_t byte1, uint8_t byte2) {
    start();
    write_byte(byte1);
    write_byte(byte2);
    stop();
  }

  void start() {
    sda_pin_->digital_write(true);
    scl_pin_->digital_write(true);
    delayMicroseconds(2);
    sda_pin_->digital_write(false); // SCL高电平期间SDA下降沿为启动信号 [cite: 134]
    delayMicroseconds(2);
    scl_pin_->digital_write(false);
  }

  void stop() {
    sda_pin_->digital_write(false);
    scl_pin_->digital_write(true);
    delayMicroseconds(2);
    sda_pin_->digital_write(true);  // SCL高电平期间SDA上升沿为停止信号 [cite: 134]
    delayMicroseconds(2);
  }

  void write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
      sda_pin_->digital_write(data & 0x80);
      delayMicroseconds(2);
      scl_pin_->digital_write(true); // 上升沿输入数据 [cite: 133]
      delayMicroseconds(2);
      scl_pin_->digital_write(false);
      data <<= 1;
    }
    // 应答位处理：手册规定应答1和2固定为1 
    sda_pin_->digital_write(true);
    scl_pin_->digital_write(true);
    delayMicroseconds(2);
    scl_pin_->digital_write(false);
  }
};