# VibeLight 5 changelog

## [6.0.0] - 2023-08-21

### Changed
- Updated Arduino IDE version to 2.1.1
- Implemented support for latest Home Assistant (`white_value` was removed in version "2021.7", see https://developers.home-assistant.io/blog/2022/08/18/light_white_value_removed/)

## [5.2.1] - 2021-08-30

### Fixed
- White value was not settable if no other color values were sent

## [5.2.0] - 2021-08-29

### Added
- Support for WS2812B LED type
- Animation support (dependent on LED type)
