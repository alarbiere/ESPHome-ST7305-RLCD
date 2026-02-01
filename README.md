# ST7305 RLCD Driver for ESPHome

ESPHome display driver for the ST7305 reflective LCD controller.

**Supported Hardware:** [Waveshare ESP32-S3-RLCD-4.2](https://docs.waveshare.com/ESP32-S3-RLCD-4.2) and similar displays.\
**Note** *This module was vibe coded* 

## Features

- 400x300 pixel resolution
- Rotation
  - 0° landscape buttons on top (default)
  - 90° portrait buttons left
  - 180° landscape buttons bottom
  - 270° portrait buttons right
- 1-bit monochrome (black/white)
- Full ESPHome display API

## Installation 

Set this repository as an external component in your ESPHome device YAML.

```yaml
external_components:
  - source: github://kylehase/ESPHome-ST7305-RLCD
    components: [ st7305_rlcd ]
```

## Example Configuration

```yaml
spi:
  clk_pin: GPIO11
  mosi_pin: GPIO12

# =============================================================================
# Font Configuration
# =============================================================================
font:
  - file: "gfonts://Roboto"
    id: roboto
    size: 40

# =============================================================================
# Display Configuration
# =============================================================================
display:
  - platform: st7305_rlcd
    rotation: 0
    id: my_display
    cs_pin: GPIO40
    dc_pin: GPIO5
    reset_pin: GPIO41
    update_interval: 1s
    
    lambda: |-
      // Get dimensions
      int w = it.get_width();
      int h = it.get_height();

      // Print "Hello World!" in the center of the screen
      // Arguments: x, y, font_id, color, alignment, text
      it.printf(w / 2, h / 2, id(roboto), COLOR_ON, TextAlign::CENTER, "Hello World!");

```

## Pin Configuration (Waveshare ESP32-S3-RLCD-4.2)

| Signal | GPIO |
|--------|------|
| SCLK   | 11   |
| MOSI   | 12   |
| CS     | 40   |
| DC     | 5    |
| RST    | 41   |

## Colors

- `COLOR_ON` = Black
- `COLOR_OFF` = White

## License

Apache 2.0 - See LICENSE file
