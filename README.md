# VibeLight 5.0 (based on ESP8266 for Home Assistant)

Example configuration:

  light:
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
