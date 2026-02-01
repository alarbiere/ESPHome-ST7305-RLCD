/**
 * @file st7305_rlcd.h
 * @brief ESPHome driver for ST7305 reflective LCD (Waveshare ESP32-S3-RLCD-4.2)
 * 
 * Display: 400x300 pixels, 1-bit monochrome, SPI interface
 * Supports rotation: 0°, 90°, 180°, 270°
 * 
 * @version 1.0.0
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace st7305_rlcd {

// Physical display dimensions (native landscape orientation)
static const uint16_t ST7305_WIDTH = 400;
static const uint16_t ST7305_HEIGHT = 300;
static const size_t ST7305_BUFFER_SIZE = (ST7305_WIDTH * ST7305_HEIGHT) / 8;

class ST7305RLCD : public display::DisplayBuffer,
                   public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                         spi::CLOCK_POLARITY_LOW,
                                         spi::CLOCK_PHASE_LEADING,
                                         spi::DATA_RATE_10MHZ> {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  void fill(Color color) override;

  void set_dc_pin(GPIOPin *pin) { this->dc_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }

  display::DisplayType get_display_type() override {
    return display::DisplayType::DISPLAY_TYPE_BINARY;
  }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override { return ST7305_WIDTH; }
  int get_height_internal() override { return ST7305_HEIGHT; }
  size_t get_buffer_length_() { return ST7305_BUFFER_SIZE; }

 private:
  void hardware_reset_();
  void init_display_();
  void init_pixel_lut_();
  void write_display_();

  void send_command_(uint8_t cmd);
  void send_data_(uint8_t data);

  GPIOPin *dc_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};

  // Pixel coordinate lookup tables for O(1) buffer access
  uint16_t *pixel_index_lut_{nullptr};
  uint8_t *pixel_bit_lut_{nullptr};
};

}  // namespace st7305_rlcd
}  // namespace esphome
