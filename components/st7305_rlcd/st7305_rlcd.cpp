/**
 * @file st7305_rlcd.cpp
 * @brief Implementation of ST7305 RLCD driver for ESPHome
 */

#include "st7305_rlcd.h"
#include "esphome/core/log.h"
#include <cstring>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

namespace esphome {
namespace st7305_rlcd {

// Import display types into namespace
using display::COLOR_OFF;
using display::COLOR_ON;
using display::Rect;

static const char *const TAG = "st7305_rlcd";

// =============================================================================
// Component Lifecycle
// =============================================================================

void ST7305RLCD::setup() {
  ESP_LOGI(TAG, "Initializing ST7305 RLCD (400x300)");

  // Validate and configure DC pin
  if (this->dc_pin_ == nullptr) {
    ESP_LOGE(TAG, "DC pin not configured");
    this->mark_failed();
    return;
  }
  this->dc_pin_->setup();
  this->dc_pin_->digital_write(false);

  // Configure reset pin if provided
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
  }

  // Initialize SPI
  this->spi_setup();

  // Hardware initialization
  this->hardware_reset_();
  this->init_display_();

  // Allocate display buffer
  this->init_internal_(ST7305_BUFFER_SIZE);
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Buffer allocation failed");
    this->mark_failed();
    return;
  }

  // Initialize pixel lookup tables
  this->init_pixel_lut_();
  if (this->pixel_index_lut_ == nullptr || this->pixel_bit_lut_ == nullptr) {
    ESP_LOGE(TAG, "LUT allocation failed");
    this->mark_failed();
    return;
  }

  // Clear display
  this->fill(COLOR_OFF);
  this->write_display_();

  ESP_LOGI(TAG, "Initialization complete");
}

void ST7305RLCD::dump_config() {
  LOG_DISPLAY("", "ST7305 RLCD", this);
  ESP_LOGCONFIG(TAG, "  Resolution: %dx%d", this->get_width(), this->get_height());
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_UPDATE_INTERVAL(this);
}

void ST7305RLCD::update() {
  if (this->buffer_ == nullptr)
    return;

  // Clear buffer to white
  std::memset(this->buffer_, 0xFF, ST7305_BUFFER_SIZE);

  // Set up clipping using rotated dimensions (get_width/get_height handle rotation)
  this->start_clipping(Rect(0, 0, this->get_width(), this->get_height()));

  // Execute user's lambda
  if (this->writer_.has_value()) {
    (*this->writer_)(*this);
  }

  this->end_clipping();

  // Transfer buffer to display
  this->write_display_();
}

void ST7305RLCD::fill(Color color) {
  if (this->buffer_ == nullptr)
    return;
  // 0xFF = white (all bits set), 0x00 = black (all bits clear)
  std::memset(this->buffer_, color.is_on() ? 0x00 : 0xFF, ST7305_BUFFER_SIZE);
}

// =============================================================================
// Hardware Reset
// =============================================================================

void ST7305RLCD::hardware_reset_() {
  if (this->reset_pin_ == nullptr)
    return;

  this->reset_pin_->digital_write(true);
  delay(10);
  this->reset_pin_->digital_write(false);
  delay(10);
  this->reset_pin_->digital_write(true);
  delay(120);
}

// =============================================================================
// Display Initialization
// CRITICAL: This sequence is hardware-specific and must match the panel.
// Values derived from working Waveshare Arduino driver.
// =============================================================================

void ST7305RLCD::init_display_() {
  // NVM Load Control
  this->send_command_(0xD6);
  this->send_data_(0x17);
  this->send_data_(0x02);

  // Booster Enable
  this->send_command_(0xD1);
  this->send_data_(0x01);

  // Gate Voltage Control
  this->send_command_(0xC0);
  this->send_data_(0x11);
  this->send_data_(0x04);

  // VSHP Setting
  this->send_command_(0xC1);
  this->send_data_(0x69);
  this->send_data_(0x69);
  this->send_data_(0x69);
  this->send_data_(0x69);

  // VSLP Setting
  this->send_command_(0xC2);
  this->send_data_(0x19);
  this->send_data_(0x19);
  this->send_data_(0x19);
  this->send_data_(0x19);

  // VSHN Setting
  this->send_command_(0xC4);
  this->send_data_(0x4B);
  this->send_data_(0x4B);
  this->send_data_(0x4B);
  this->send_data_(0x4B);

  // VSLN Setting
  this->send_command_(0xC5);
  this->send_data_(0x19);
  this->send_data_(0x19);
  this->send_data_(0x19);
  this->send_data_(0x19);

  // OSC Setting
  this->send_command_(0xD8);
  this->send_data_(0x80);
  this->send_data_(0xE9);

  // Frame Rate Control
  this->send_command_(0xB2);
  this->send_data_(0x02);

  // Gate EQ Control (High Power Mode)
  this->send_command_(0xB3);
  this->send_data_(0xE5);
  this->send_data_(0xF6);
  this->send_data_(0x05);
  this->send_data_(0x46);
  this->send_data_(0x77);
  this->send_data_(0x77);
  this->send_data_(0x77);
  this->send_data_(0x77);
  this->send_data_(0x76);
  this->send_data_(0x45);

  // Gate EQ Control (Low Power Mode)
  this->send_command_(0xB4);
  this->send_data_(0x05);
  this->send_data_(0x46);
  this->send_data_(0x77);
  this->send_data_(0x77);
  this->send_data_(0x77);
  this->send_data_(0x77);
  this->send_data_(0x76);
  this->send_data_(0x45);

  // Unknown command 0x62
  this->send_command_(0x62);
  this->send_data_(0x32);
  this->send_data_(0x03);
  this->send_data_(0x1F);

  // Source EQ Enable
  this->send_command_(0xB7);
  this->send_data_(0x13);

  // Gate Line Setting
  this->send_command_(0xB0);
  this->send_data_(0x64);

  // Sleep Out
  this->send_command_(0x11);
  delay(200);

  // VSHL Select
  this->send_command_(0xC9);
  this->send_data_(0x00);

  // Memory Data Access Control
  this->send_command_(0x36);
  this->send_data_(0x48);

  // Data Format Select (1-bit mode)
  this->send_command_(0x3A);
  this->send_data_(0x11);

  // Gamma Mode Setting
  this->send_command_(0xB9);
  this->send_data_(0x20);

  // Panel Setting
  this->send_command_(0xB8);
  this->send_data_(0x29);

  // Display Inversion On
  this->send_command_(0x21);

  // Column Address Set
  this->send_command_(0x2A);
  this->send_data_(0x12);
  this->send_data_(0x2A);

  // Row Address Set
  this->send_command_(0x2B);
  this->send_data_(0x00);
  this->send_data_(0xC7);

  // Tearing Effect Line On
  this->send_command_(0x35);
  this->send_data_(0x00);

  // Auto Power Down Control
  this->send_command_(0xD0);
  this->send_data_(0xFF);

  // High Power Mode
  this->send_command_(0x38);

  // Display On
  this->send_command_(0x29);
}

// =============================================================================
// Pixel Lookup Table Initialization
// CRITICAL: Uses x * HEIGHT + y indexing to match Arduino's [x][y] array layout
// =============================================================================

void ST7305RLCD::init_pixel_lut_() {
  const size_t total_pixels = ST7305_WIDTH * ST7305_HEIGHT;
  const size_t lut_bytes = total_pixels * (sizeof(uint16_t) + sizeof(uint8_t));

#ifdef USE_ESP32
  // Try PSRAM first for large allocation
  size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  if (free_psram > lut_bytes) {
    ESP_LOGD(TAG, "Allocating LUTs in PSRAM");
    this->pixel_index_lut_ = (uint16_t *)heap_caps_malloc(
        total_pixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    this->pixel_bit_lut_ = (uint8_t *)heap_caps_malloc(
        total_pixels * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  }
#endif

  // Fallback to regular heap
  if (this->pixel_index_lut_ == nullptr) {
    this->pixel_index_lut_ = new (std::nothrow) uint16_t[total_pixels];
  }
  if (this->pixel_bit_lut_ == nullptr) {
    this->pixel_bit_lut_ = new (std::nothrow) uint8_t[total_pixels];
  }

  if (this->pixel_index_lut_ == nullptr || this->pixel_bit_lut_ == nullptr) {
    return;
  }

  // Build lookup tables using ST7305's 2x4 block pixel arrangement
  // This matches the Arduino driver's InitLandscapeLUT() exactly
  //
  // The ST7305 organizes pixels in 2x4 blocks:
  // - Each byte contains 8 pixels (2 wide x 4 tall)
  // - 200 horizontal blocks (400 pixels / 2)
  // - 75 vertical blocks (300 pixels / 4)
  // - Total: 200 * 75 = 15000 bytes
  
  const uint16_t H4 = ST7305_HEIGHT >> 2;  // 300/4 = 75 vertical blocks

  for (uint16_t y = 0; y < ST7305_HEIGHT; y++) {
    uint16_t inv_y = ST7305_HEIGHT - 1 - y;  // Invert Y coordinate
    uint16_t block_y = inv_y >> 2;           // Which vertical block (0-74)
    uint8_t local_y = inv_y & 3;             // Position within block (0-3)

    for (uint16_t x = 0; x < ST7305_WIDTH; x++) {
      uint16_t byte_x = x >> 1;              // Which horizontal block (0-199)
      uint8_t local_x = x & 1;               // Position within block (0-1)

      // Buffer index: column-major within blocks
      uint32_t buffer_idx = byte_x * H4 + block_y;
      
      // Bit position: 2x4 arrangement within byte
      // Bit 7: (local_y=0, local_x=0), Bit 6: (local_y=0, local_x=1)
      // Bit 5: (local_y=1, local_x=0), Bit 4: (local_y=1, local_x=1)
      // ...
      uint8_t bit = 7 - ((local_y << 1) | local_x);

      // Store in LUT using column-major order (x * HEIGHT + y)
      const uint32_t lut_pos = (uint32_t)x * ST7305_HEIGHT + y;
      this->pixel_index_lut_[lut_pos] = buffer_idx;
      this->pixel_bit_lut_[lut_pos] = (1 << bit);
    }
  }
}

// =============================================================================
// Pixel Drawing
// =============================================================================

void HOT ST7305RLCD::draw_absolute_pixel_internal(int x, int y, Color color) {
  // Bounds check
  if (x < 0 || x >= ST7305_WIDTH || y < 0 || y >= ST7305_HEIGHT)
    return;
  if (this->pixel_index_lut_ == nullptr)
    return;

  const uint32_t lut_pos = (uint32_t)x * ST7305_HEIGHT + y;
  const uint16_t buf_idx = this->pixel_index_lut_[lut_pos];
  const uint8_t bit_mask = this->pixel_bit_lut_[lut_pos];

  // Bit set = white, bit clear = black
  if (color.is_on()) {
    this->buffer_[buf_idx] &= ~bit_mask;  // BLACK
  } else {
    this->buffer_[buf_idx] |= bit_mask;   // WHITE
  }
}

// =============================================================================
// Display Transfer
// CRITICAL: Memory write command and data must be sent with CS held LOW
// =============================================================================

void ST7305RLCD::write_display_() {
  if (this->buffer_ == nullptr)
    return;

  // Wake display
  this->send_command_(0x38);  // High Power Mode
  this->send_command_(0x29);  // Display On

  // Set address window
  this->send_command_(0x2A);  // Column address
  this->send_data_(0x12);
  this->send_data_(0x2A);

  this->send_command_(0x2B);  // Row address
  this->send_data_(0x00);
  this->send_data_(0xC7);

  // CRITICAL: Memory write - CS must stay LOW for command + all data
  this->dc_pin_->digital_write(false);  // Command mode
  this->enable();                        // CS LOW
  this->write_byte(0x2C);               // Memory Write command

  this->dc_pin_->digital_write(true);   // Data mode (CS still LOW)
  this->write_array(this->buffer_, ST7305_BUFFER_SIZE);
  this->disable();                       // CS HIGH
}

// =============================================================================
// SPI Helpers
// =============================================================================

void ST7305RLCD::send_command_(uint8_t cmd) {
  this->dc_pin_->digital_write(false);
  this->enable();
  this->write_byte(cmd);
  this->disable();
}

void ST7305RLCD::send_data_(uint8_t data) {
  this->dc_pin_->digital_write(true);
  this->enable();
  this->write_byte(data);
  this->disable();
}

}  // namespace st7305_rlcd
}  // namespace esphome
