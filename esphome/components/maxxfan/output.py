"""MaxxFan output component for ESPHome."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.components import globals as globals_comp
from esphome.const import CONF_ID

from . import maxxfan_ns

# The component reads/writes the shared YAML globals, so it needs that component.
DEPENDENCIES = ["globals"]

# Define the output classes
SpeedOutput = maxxfan_ns.class_("SpeedOutput", cg.Component, output.FloatOutput)
PowerOutput = maxxfan_ns.class_("PowerOutput", cg.Component, output.BinaryOutput)
DirectionOutput = maxxfan_ns.class_("DirectionOutput", cg.Component, output.BinaryOutput)
CoverOutput = maxxfan_ns.class_("CoverOutput", cg.Component, output.BinaryOutput)

# Configuration schema for each output type
SPEED_OUTPUT_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(SpeedOutput),
}).extend(cv.COMPONENT_SCHEMA)

POWER_OUTPUT_SCHEMA = output.BINARY_OUTPUT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(PowerOutput),
}).extend(cv.COMPONENT_SCHEMA)

DIRECTION_OUTPUT_SCHEMA = output.BINARY_OUTPUT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(DirectionOutput),
}).extend(cv.COMPONENT_SCHEMA)

COVER_OUTPUT_SCHEMA = output.BINARY_OUTPUT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(CoverOutput),
}).extend(cv.COMPONENT_SCHEMA)

# IDs of the shared YAML globals that hold the single source of truth for fan
# state. They are bound to the component's pointers in to_code().
CONF_ACTUAL_FAN_SPEED = "actual_fan_speed_id"
CONF_ACTUAL_FAN_POWER = "actual_fan_power_id"
CONF_ACTUAL_FAN_DIRECTION = "actual_fan_direction_id"
CONF_ACTUAL_FAN_COVER = "actual_fan_cover_id"
CONF_IS_BOOT_FINISHED = "is_boot_finished_id"

# Register the output platforms
CONFIG_SCHEMA = cv.Schema({
    cv.Optional("speed"): SPEED_OUTPUT_SCHEMA,
    cv.Optional("power"): POWER_OUTPUT_SCHEMA,
    cv.Optional("direction"): DIRECTION_OUTPUT_SCHEMA,
    cv.Optional("cover"): COVER_OUTPUT_SCHEMA,
    # Every YAML global registers its ID as GlobalsComponent (the templated
    # restoring type is only applied to the C++ variable), so all five validate
    # against GlobalsComponent. get_variable() still yields the real typed
    # pointer, which matches link_state_globals()'s precise signatures.
    cv.Required(CONF_ACTUAL_FAN_SPEED): cv.use_id(globals_comp.GlobalsComponent),
    cv.Required(CONF_ACTUAL_FAN_POWER): cv.use_id(globals_comp.GlobalsComponent),
    cv.Required(CONF_ACTUAL_FAN_DIRECTION): cv.use_id(globals_comp.GlobalsComponent),
    cv.Required(CONF_ACTUAL_FAN_COVER): cv.use_id(globals_comp.GlobalsComponent),
    cv.Required(CONF_IS_BOOT_FINISHED): cv.use_id(globals_comp.GlobalsComponent),
})


async def to_code(config):
    # Bind the component's state pointers to the shared YAML globals.
    speed_g = await cg.get_variable(config[CONF_ACTUAL_FAN_SPEED])
    power_g = await cg.get_variable(config[CONF_ACTUAL_FAN_POWER])
    direction_g = await cg.get_variable(config[CONF_ACTUAL_FAN_DIRECTION])
    cover_g = await cg.get_variable(config[CONF_ACTUAL_FAN_COVER])
    boot_g = await cg.get_variable(config[CONF_IS_BOOT_FINISHED])
    cg.add(maxxfan_ns.link_state_globals(speed_g, power_g, direction_g, cover_g, boot_g))

    if "speed" in config:
        var = cg.new_Pvariable(config["speed"][CONF_ID])
        await cg.register_component(var, config["speed"])
        await output.register_output(var, config["speed"])

    if "power" in config:
        var = cg.new_Pvariable(config["power"][CONF_ID])
        await cg.register_component(var, config["power"])
        await output.register_output(var, config["power"])

    if "direction" in config:
        var = cg.new_Pvariable(config["direction"][CONF_ID])
        await cg.register_component(var, config["direction"])
        await output.register_output(var, config["direction"])

    if "cover" in config:
        var = cg.new_Pvariable(config["cover"][CONF_ID])
        await cg.register_component(var, config["cover"])
        await output.register_output(var, config["cover"])
