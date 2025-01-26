import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.components import sensor, uart
from esphome.const import (
    CONF_CO2,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_CARBON_DIOXIDE,
    ICON_MOLECULE_CO2,
    STATE_CLASS_MEASUREMENT,
    UNIT_PARTS_PER_MILLION,
    UNIT_CELSIUS,
)

DEPENDENCIES = ["uart"]

CONF_AUTOMATIC_BASELINE_CALIBRATION = "automatic_baseline_calibration"
CONF_WARMUP_TIME = "warmup_time"

cm1108_ns = cg.esphome_ns.namespace("cm1108")
CM1108Component = cm1108_ns.class_("CM1108Component", cg.PollingComponent, uart.UARTDevice)
CM1108CalibrateZeroAction = cm1108_ns.class_(
    "CM1108CalibrateZeroAction", automation.Action
)
CM1108ABCEnableAction = cm1108_ns.class_("CM1108ABCEnableAction", automation.Action)
CM1108ABCDisableAction = cm1108_ns.class_("CM1108ABCDisableAction", automation.Action)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CM1108Component),
            cv.Required(CONF_CO2): sensor.sensor_schema(
                unit_of_measurement=UNIT_PARTS_PER_MILLION,
                icon=ICON_MOLECULE_CO2,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_CARBON_DIOXIDE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_AUTOMATIC_BASELINE_CALIBRATION): cv.boolean,
            cv.Optional(
                CONF_WARMUP_TIME, default="75s"
            ): cv.positive_time_period_seconds,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_CO2 in config:
        sens = await sensor.new_sensor(config[CONF_CO2])
        cg.add(var.set_co2_sensor(sens))

    if CONF_AUTOMATIC_BASELINE_CALIBRATION in config:
        cg.add(var.set_abc_enabled(config[CONF_AUTOMATIC_BASELINE_CALIBRATION]))

    cg.add(var.set_warmup_seconds(config[CONF_WARMUP_TIME]))


CALIBRATION_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(CM1108Component),
    }
)


@automation.register_action(
    "cm1108.calibrate_zero", CM1108CalibrateZeroAction, CALIBRATION_ACTION_SCHEMA
)
@automation.register_action(
    "cm1108.abc_enable", CM1108ABCEnableAction, CALIBRATION_ACTION_SCHEMA
)
@automation.register_action(
    "cm1108.abc_disable", CM1108ABCDisableAction, CALIBRATION_ACTION_SCHEMA
)
async def cm1108_calibration_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)