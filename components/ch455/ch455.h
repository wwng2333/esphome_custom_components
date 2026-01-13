#include "esphome.h"

using namespace esphome;

// CH455 寄存器与命令定义
static const uint8_t CH455_CMD_SYSTEM = 0x48; // 设置系统参数命令 
static const uint8_t CH455_DIG0_ADDR   = 0x68; // DIG0 驱动地址 

class CH455Component : public PollingComponent, public display::DisplayBuffer {
 public:
  // 构造函数：指定引脚和刷新频率
  CH455Component(InternalGPIOPin *scl, InternalGPIOPin *sda) : PollingComponent(1000) {
    scl_pin_ = scl;
    sda_pin_ = sda;
  }

  void setup() override {
    scl_pin_->setup();
    sda_pin_->setup();
    scl_pin_->digital_write(true);
    sda_pin_->digital_write(true);

    // 初始化：开启显示(ENA=1)，8段模式(7SEG=0)，最大亮度(INTENS=000B) [cite: 163, 166]
    // 字节2格式: [KOFF][INTENS][7SEG][SLEEP]0[ENA]B -> 00000001B (0x01)
    send_command(CH455_CMD_SYSTEM, 0x01);
  }

  void update() override {
    this->display(); // 触发 DisplayBuffer 的绘制逻辑
  }

  // 这里的 draw_absolute_pixel_internal 在数码管中通常不直接操作
  void draw_absolute_pixel_internal(int x, int y, Color color) override {}

  // 将缓冲区数据写入芯片 
  void display() {
    for (uint8_t i = 0; i < 4; i++) {
      // CH455 地址：68H, 6AH, 6CH, 6EH 分别对应 DIG0~DIG3 
      send_command(CH455_DIG0_ADDR + (i * 2), buffer_[i]);
    }
  }

  // 设置亮度 (0-7级) [cite: 166]
  void set_brightness(uint8_t level) {
    uint8_t intens = (level & 0x07) << 4; 
    send_command(CH455_CMD_SYSTEM, intens | 0x01);
  }

  int get_width_internal() override { return 4; }
  int get_height_internal() override { return 1; }

 protected:
  InternalGPIOPin *scl_pin_;
  InternalGPIOPin *sda_pin_;
  uint8_t buffer_[4] = {0x00, 0x00, 0x00, 0x00}; // 对应4个数码管的段码 [cite: 103]

  // 核心通讯逻辑：写操作 6 步骤 [cite: 137]
  void send_command(uint8_t byte1, uint8_t byte2) {
    start_signal();
    write_byte(byte1);
    write_byte(byte2);
    stop_signal();
  }

  void start_signal() {
    sda_pin_->digital_write(true);
    scl_pin_->digital_write(true);
    delay_us(2);
    sda_pin_->digital_write(false); // SCL高电平时SDA下降沿为启动 [cite: 134]
    delay_us(2);
    scl_pin_->digital_write(false);
  }

  void stop_signal() {
    sda_pin_->digital_write(false);
    scl_pin_->digital_write(true);
    delay_us(2);
    sda_pin_->digital_write(true);  // SCL高电平时SDA上升沿为停止 [cite: 134]
    delay_us(2);
  }

  void write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
      sda_pin_->digital_write(data & 0x80);
      delay_us(2);
      scl_pin_->digital_write(true);  // 上升沿采样 [cite: 133]
      delay_us(2);
      scl_pin_->digital_write(false);
      data <<= 1;
    }
    // 应答位：手册规定应答1和2总是固定为1 [cite: 137]
    sda_pin_->digital_write(true);
    scl_pin_->digital_write(true);
    delay_us(2);
    scl_pin_->digital_write(false);
  }

  void delay_us(uint32_t us) { machine_delay_us(us); }
};