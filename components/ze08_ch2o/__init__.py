import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UART_ID

DEPENDENCIES = ['uart']
AUTO_LOAD = ['sensor']

# 定义命名空间
ze08_ch2o_ns = cg.esphome_ns.namespace('ze08_ch2o')

# 修改点 1: 将 cg.PollingComponent 改为 cg.Component
# 这告诉代码生成器，C++ 类继承自 Component 而非轮询类
ZE08CH2OSensor = ze08_ch2o_ns.class_('ZE08CH2OSensor', cg.Component, uart.UARTDevice)

CONF_ZE08_ID = 'ze08_ch2o_id'

# 修改点 2: 将 cv.polling_component_schema('5s') 改为 cv.COMPONENT_SCHEMA
# 这样会移除 YAML 中对 update_interval 的强制要求，并防止生成 set_update_interval 调用
CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(ZE08CH2OSensor),
    cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    # 注册组件，现在它会作为普通 Component 注册
    await cg.register_component(var, config)
    
    # 注册 UART 设备
    await uart.register_uart_device(var, config)
