# ST7305 RLCD Driver for ESPHome

ESPHome display driver for the ST7305 reflective LCD controller. Ported from the [Waveshare reference driver](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2/tree/main/Example/XiaoZhi/XiaoZhiCode_V2.1.0/main/boards/waveshare-s3-rlcd-4.2).

**Supported Hardware:** 
| Model | Resolution | Size | Tested | Compatible Panels | 
|-------|------------|------|-------------|-------------------|
| `WAVESHARE_400X300` | 400×300 | 4.2" | Yes | GooDisplay GDTL042T71 |
| `OSPTEK_200X200` | 200×200 | 1.54" | No | Osptek YDP154H008 |
| `CUSTOM` | User-defined | - | N/A | Other ST7305 panels |

### Panel Equivalents

Many ST7305 panels from different manufacturers share identical specifications:

- **4.2" Landscape (400×300)**: Waveshare ESP32-S3-RLCD-4.2, GooDisplay GDTL042T71
- **1.54" Square (200×200)**: Osptek YDP154H008

If your panel matches the resolution and orientation of a predefined model, you should be able to use that model even if the manufacturer differs. Otherwise, try `CUSTOM`.


**Note** *This module was vibe coded* 

![PXL_20260201_010637447 RAW-01 COVER-rs](https://github.com/user-attachments/assets/48f37f0f-299a-475a-80f8-cf618c1e873c)


## Features

- Custom panel support for unlisted ST7305 displays
- Full ESPHome display API (print, line, rectangle, circle, etc.)
- Rotation support: 0°, 90°, 180°, 270°
- Power management for battery optimization

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
    model: WAVESHARE_400X300
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
## Configuration Options

| Option | Required | Default | Description |
|--------|----------|---------|-------------|
| `model` | No | `WAVESHARE_400X300` | Panel model |
| `cs_pin` | Yes | - | SPI chip select pin |
| `dc_pin` | Yes | - | Data/Command selection pin |
| `reset_pin` | No | - | Hardware reset pin |
| `rotation` | No | 0 | Display rotation (0, 90, 180, 270) |
| `update_interval` | No | 1s | How often to refresh display |

### Custom Panel Options

When `model: CUSTOM`, these are required:

| Option | Description |
|--------|-------------|
| `width` | Panel width in pixels |
| `height` | Panel height in pixels |
| `orientation` | `LANDSCAPE` (2×4 blocks) or `PORTRAIT` (4×2 blocks) |

## Rotation

| Setting | Description |
|---------|-------------|
| `rotation: 0` | Default orientation |
| `rotation: 90` | Rotated 90° clockwise |
| `rotation: 180` | Upside-down |
| `rotation: 270` | Rotated 270° clockwise |

Use `it.get_width()` and `it.get_height()` in lambda for rotation-aware dimensions.

## Power Management

The ST7305 is a reflective LCD - content is retained in low-power states with no backlight required.

### Battery Optimization Pattern

```yaml
display:
  - platform: st7305_rlcd
    id: my_display
    model: WAVESHARE_400X300
    cs_pin: GPIO40
    dc_pin: GPIO5
    reset_pin: GPIO41
    update_interval: never  # Manual updates only

interval:
  - interval: 60s
    then:
      - lambda: id(my_display).wake();
      - component.update: my_display
      - delay: 200ms
      - lambda: id(my_display).sleep();
```

### Power Methods

| Method | Power | Description |
|--------|-------|-------------|
| `sleep()` | ~10µA | Lowest power, 120ms wake delay |
| `wake()` | - | Exit sleep mode |
| `low_power_mode()` | ~1mA | ~1Hz refresh, for static content |
| `high_power_mode()` | ~5mA | ~51Hz refresh, for animations |
| `display_on()` | - | Turn display on |
| `display_off()` | Low | Turn display off, RAM retained |

## Colors

- `COLOR_ON` = Black (pixel on)
- `COLOR_OFF` = White (pixel off)

## Technical Details

### Pixel Block Structure

ST7305 panels pack 8 pixels per byte in different arrangements:

**Landscape (400×300 Waveshare):** 2 columns × 4 rows per byte
```
Bit 7: (row 0, col 0)  Bit 6: (row 0, col 1)
Bit 5: (row 1, col 0)  Bit 4: (row 1, col 1)
Bit 3: (row 2, col 0)  Bit 2: (row 2, col 1)
Bit 1: (row 3, col 0)  Bit 0: (row 3, col 1)
```

**Portrait (200×200 Osptek):** 4 columns × 2 rows per byte
```
Bit 7: (row 0, col 0)  Bit 6: (row 0, col 1)  Bit 5: (row 0, col 2)  Bit 4: (row 0, col 3)
Bit 3: (row 1, col 0)  Bit 2: (row 1, col 1)  Bit 1: (row 1, col 2)  Bit 0: (row 1, col 3)
```

### Memory Usage

| Model | Resolution | Buffer | LUTs (PSRAM) |
|-------|------------|--------|--------------|
| Waveshare | 400×300 | 15KB | ~360KB |
| Osptek | 200×200 | 5KB | ~120KB |

**Note:** Requires ESP32 with PSRAM for lookup tables.

### Pin Configuration - Waveshare ESP32-S3-RLCD-4.2

```yaml
spi:
  clk_pin: GPIO39
  mosi_pin: GPIO38

display:
  - platform: st7305_rlcd
    model: WAVESHARE_400X300
    cs_pin: GPIO40
    dc_pin: GPIO5
    reset_pin: GPIO41
```

## Troubleshooting

### Display shows nothing
1. Check SPI wiring (CLK, MOSI, CS, DC)
2. Verify reset pin is connected
3. Check power supply (3.3V)

### Display shows garbage
1. Verify correct model is selected
2. Check pixel block orientation matches panel

### Custom panel doesn't work
1. Ensure width/height are correct
2. Try both LANDSCAPE and PORTRAIT orientations
3. Check panel uses ST7305 controller (not ST7306, etc.)

## Version History

- **v2.0.0** - Multi-panel support (Waveshare, Osptek, Custom), selectable power modes
- **v1.0.0** - Initial release (Waveshare 400×300 only)

## References

### Datasheets
- [ST7305 Controller Datasheet](https://files.waveshare.com/wiki/common/ST_7305_V0_2.pdf)
- [Osptek YDP154H008 (200×200)](https://admin.osptek.com/uploads/YDP_154_H008_V3_c24b455ff9.pdf)

### Development Boards
- [Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.com/wiki/ESP32-S3-RLCD-4.2)
- [GooDisplay GDTL042T71 (400×300)](https://www.good-display.com/product/455.html)

## License

Apache 2.0 - See LICENSE file
