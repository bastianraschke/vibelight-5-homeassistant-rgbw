# VibeLight 5 (based on ESP8266 for Home Assistant)

## Features

- RGB(W) LED light firmware for ESP8266 (e.g. Wemos D1 Mini)
- Easy integration to Home Assistant
- Encrypted MQTT communication with TLS 1.1
- Supports both RGB and RGBW LED lights
- Supports transitions for Home Assistant
- **New:** Support for both classic cathode and WS2812B LED types
- **New:** Supports animations (see examples below) - dependent of LED type
- Customizable color channel offsets for color correction of the LED strip

## Project application

Here you see the firmware used on a Wemos D1 Mini based RGBW controller to nicely light up my alcohol collection:

<img alt="VibeLight 5.0 nicely lights up my alcohol collection" src="https://github.com/bastianraschke/vibelight-5-homeassistant-rgbw/blob/master/projectcover.jpg" width="650">

## Configuration

The firmware must be configured before flashing to ESP8266. Rename `src/Firmware/sketches/Production/config-sample.h` to `src/Firmware/sketches/Production/config.h` and change the values like desired.

### Flash settings

#### NodeMCU

* **Arduino IDE:** 2.1.1
* **Platform:** esp8266 2.4.2
* **Board:** NodeMCU 1.0 (ESP-12E Module)
* **CPU Frequency**: 80 MHz
* **Debug Port**: Disabled
* **Debug Level**: None
* **Erase Flash**: Only Sketch
* **Flash Size**: 4M (1M SPIFFS)
* **IwIP Variant**: v2 Lower Memory
* **VTables**: Flash

#### LOLIN (Wemos) D1 mini

* **Arduino IDE:** 2.1.1
* **Platform:** esp8266 2.4.2
* **Board:** LOLIN (WEMOS) D1 R2 & mini
* **CPU Frequency**: 80 MHz
* **Debug Port**: Disabled
* **Debug Level**: None
* **Erase Flash**: Only Sketch
* **Flash Size**: 4M (1M SPIFFS)
* **IwIP Variant**: v2 Lower Memory
* **VTables**: Flash

## Example configuration for Home Assistant

Tested with Home Assistant 2023.8.2.

The example blocks must be added to the `mqtt` block of your configuration.

### RGB (classic cathode LEDs)

    - light:
        name: "My VibeLight RGB light"
        schema: json
        state_topic: "/vibelight/api/6/id/AAAABBBB/state/"
        command_topic: "/vibelight/api/6/id/AAAABBBB/command/"
        brightness: true
        color_mode: true
        supported_color_modes: ["rgb"]
        effect: true
        effect_list:
          - none
          - colorloop

### RGBW (classic cathode LEDs)

    - light:
        name: "My VibeLight RGB light"
        schema: json
        state_topic: "/vibelight/api/6/id/AAAABBBB/state/"
        command_topic: "/vibelight/api/6/id/AAAABBBB/command/"
        brightness: true
        color_mode: true
        supported_color_modes: ["rgbw"]
        effect: true
        effect_list:
          - none
          - colorloop

### RGB (WS2812B LEDs)

    - light:
        name: "My VibeLight RGB light"
        schema: json
        state_topic: "/vibelight/api/6/id/AAAABBBB/state/"
        command_topic: "/vibelight/api/6/id/AAAABBBB/command/"
        brightness: true
        color_mode: true
        supported_color_modes: ["rgb"]
        effect: true
        effect_list:
          - none
          - rainbow
          - colorloop
          - laserscanner

### RGBW (WS2812B LEDs)

    - light:
        name: "My VibeLight RGB light"
        schema: json
        state_topic: "/vibelight/api/6/id/AAAABBBB/state/"
        command_topic: "/vibelight/api/6/id/AAAABBBB/command/"
        brightness: true
        color_mode: true
        supported_color_modes: ["rgbw"]
        effect: true
        effect_list:
          - none
          - rainbow
          - colorloop
          - laserscanner
