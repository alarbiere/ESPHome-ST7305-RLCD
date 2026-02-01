"""
ESPHome ST7305 RLCD Display Component

Supports Waveshare ESP32-S3-RLCD-4.2 and similar 400x300 reflective LCDs.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display, spi
from esphome.const import (
    CONF_DC_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES,
    CONF_RESET_PIN,
)

DEPENDENCIES = ["spi"]

st7305_rlcd_ns = cg.esphome_ns.namespace("st7305_rlcd")

# CRITICAL: List ALL parent classes for proper ESPHome component wiring.
# Missing any of these breaks lambda registration or update scheduling.
ST7305RLCD = st7305_rlcd_ns.class_(
    "ST7305RLCD",
    cg.PollingComponent,
    spi.SPIDevice,
    display.Display,
    display.DisplayBuffer,
)

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(ST7305RLCD),
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
        }
    )
    .extend(spi.spi_device_schema(cs_pin_required=True)),
    cv.has_at_least_one_key(CONF_LAMBDA, CONF_PAGES),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    # Register display first (order matters for some ESPHome internals)
    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    # Configure pins
    dc = await cg.gpio_pin_expression(config[CONF_DC_PIN])
    cg.add(var.set_dc_pin(dc))

    if CONF_RESET_PIN in config:
        reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset))

    # CRITICAL: Explicit lambda registration required for custom components.
    # register_display() alone does not reliably set up the writer.
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
