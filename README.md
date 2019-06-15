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

## Example configuration for Home Assistant

The example blocks must be added to the `light` block of your configuration.

### RGB (classic cathode LEDs)

    - platform: mqtt_json
      name: "My VibeLight RGB light"
      state_topic: "/vibelight/api/5/id/AAAABBBB/state/"
      command_topic: "/vibelight/api/5/id/AAAABBBB/command/"
      brightness: true
      rgb: true
      white_value: false
      effect: true
      effect_list:
        - none
        - colorloop
      optimistic: false
      qos: 0


### RGBW (classic cathode LEDs)

    - platform: mqtt_json
      name: "My VibeLight RGBW light"
      state_topic: "/vibelight/api/5/id/AAAABBBB/state/"
      command_topic: "/vibelight/api/5/id/AAAABBBB/command/"
      brightness: true
      rgb: true
      white_value: true
      effect: true
      effect_list:
        - none
        - colorloop
      optimistic: false
      qos: 0

### RGB (WS2812B LEDs)

    - platform: mqtt_json
      name: "My VibeLight RGB light"
      state_topic: "/vibelight/api/5/id/AAAABBBB/state/"
      command_topic: "/vibelight/api/5/id/AAAABBBB/command/"
      brightness: true
      rgb: true
      white_value: false
      effect: true
      effect_list:
        - none
        - rainbow
        - colorloop
        - laserscanner
      optimistic: false
      qos: 0

### RGBW (WS2812B LEDs)

    - platform: mqtt_json
      name: "My VibeLight RGBW light"
      state_topic: "/vibelight/api/5/id/AAAABBBB/state/"
      command_topic: "/vibelight/api/5/id/AAAABBBB/command/"
      brightness: true
      rgb: true
      white_value: true
      effect: true
      effect_list:
        - none
        - rainbow
        - colorloop
        - laserscanner
      optimistic: false
      qos: 0
