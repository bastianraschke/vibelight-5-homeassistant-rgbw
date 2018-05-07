# VibeLight 5.0 (based on ESP8266 for Home Assistant)

## Features

- RGB(W) LED light firmware for ESP8266 (e.g. Wemos D1 Mini)
- Easy integration to Home Assistant
- Encrypted MQTT communication with TLS 1.1
- Supports both RGB and RGBW LED lights
- Supports transitions for Home Assistant
- Customizable color channel offsets for color correction of the LED strip

<img alt="VibeLight 5.0 nicely lights up my alcohol collection" src="https://github.com/bastianraschke/vibelight-5-homeassistant-rgbw/blob/master/projectcover.jpg" width="600">

## Configuration

The firmware must be configured before flashing to ESP8266. Rename `src/Firmware/sketches/Production/config-sample.h` to `src/Firmware/sketches/Production/config.h` and change the values like desired.

## Example configuration for Home Assistant

### RGBW

This example must be added to the `light` block of your configuration.

    - platform: mqtt_json
      name: "My VibeLight RGBW light"
      state_topic: "/vibelight/api/5/id/AAAABBBB/state/"
      command_topic: "/vibelight/api/5/id/AAAABBBB/command/"
      brightness: true
      rgb: true
      white_value: true
      effect: false
      optimistic: false
      qos: 0

### RGB

This example must be added to the `light` block of your configuration.

    - platform: mqtt_json
      name: "My VibeLight RGB light"
      state_topic: "/vibelight/api/5/id/AAAABBBB/state/"
      command_topic: "/vibelight/api/5/id/AAAABBBB/command/"
      brightness: true
      rgb: true
      white_value: false
      effect: false
      optimistic: false
      qos: 0
