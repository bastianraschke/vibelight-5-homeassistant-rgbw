# VibeLight 5.0 (based on ESP8266 for Home Assistant)

## Features

- RGB(W) LED light firmware for ESP8266 (Wemos D1 Mini)
- Easy integration to Home Assistant
- MQTT communication is encrypted with TLS 1.1
- Supports RGB and RGBW LED lights
- Supports transitions for Home Assistant
- Customizable color channel offsets for color correction

<img alt="VibeLight 5.0 nicely lights up my alcohol collection" src="https://github.com/bastianraschke/vibelight-5-homeassistant-rgb/blob/Development/projectcover.jpg" width="600">

## Example configuration for Home Assistant

This example must be added to the `light` block of your configuration.

    - platform: mqtt_json
      name: "My RGBW light"
      state_topic: "/vibelight/api/5/id/AAAABBBB/state/"
      command_topic: "/vibelight/api/5/id/AAAABBBB/command/"
      brightness: true
      rgb: true
      white_value: true
      effect: false
      optimistic: false
      qos: 0
