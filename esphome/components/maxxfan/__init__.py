"""MaxxFan controller component for ESPHome."""

import esphome.codegen as cg
import esphome.config_validation as cv

# Define the component namespace
maxxfan_ns = cg.esphome_ns.namespace("maxxfan")

# Empty schema - no configuration needed
CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    pass
