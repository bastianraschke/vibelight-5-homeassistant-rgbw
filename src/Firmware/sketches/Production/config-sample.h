#include "types.h"

/*
 * Configuration
 */

#define DEBUG_LEVEL                    1

// Generate a random id with:
// $ echo -n "vibelight_"; head /dev/urandom | tr -dc A-Z0-9 | head -c 8; echo ""
#define VIBELIGHT_NODE_ID              "vibelight_AAAABBBB"

#define WIFI_SSID                      ""
#define WIFI_PASSWORD                  ""

#define MQTT_CLIENTID                  VIBELIGHT_NODE_ID

// Put the host/IPv4 address of your MQTT broker here
#define MQTT_SERVER                    ""

// Use the SHA1 fingerprint of the server certificate (NOT the CA certificate) in the following format:
#define MQTT_SERVER_TLS_FINGERPRINT    "XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX"

#define MQTT_PORT                      8883
#define MQTT_USERNAME                  VIBELIGHT_NODE_ID
#define MQTT_PASSWORD                  ""

#define MQTT_CHANNEL_STATE             "/vibelight/api/5/id/AAAABBBB/state/"
#define MQTT_CHANNEL_COMMAND           "/vibelight/api/5/id/AAAABBBB/command/"

// Uncomment if on the board is an onboard LED
#define PIN_STATUSLED                  LED_BUILTIN

#define LED_TYPE                       WS2812B
#define LED_CAPABILITY                 RGB

/*
 * Define some optional offsets for color channels in the range 0..255
 * to trim some possible color inconsistency of the LED strip:
 */
#define LED_RED_OFFSET                 0
#define LED_GREEN_OFFSET               0
#define LED_BLUE_OFFSET                0
#define LED_WHITE_OFFSET               0

// Define maximum brightness value [0..100] e.g. to save energy or avoid overheating of LEDs
#define LED_MAX_BRIGHTNESS             80

// Default transition duration microseconds
#define LED_TRANSITION_DURATION        800000

/**
 * Pin configuration for LED_TYPE: CATHODE (only used if configured above)
 *
 * Set to "-1" to disable a color channel.
 *
 * For Wemos D1 mini:
 * The pin "D4" can't be used for white channel because this port is used
 * by the onboard status LED. Use "D0" instead.
 *
 * For NodeMCU:
 * The pin "D0" can't be used for white channel because this port is used
 * by the onboard status LED. Also the "D4" can't be used because it is used
 * by the ESP8266 module board LED. Use "D5" instead.
 */

#define PIN_LED_RED                    D1
#define PIN_LED_GREEN                  D2
#define PIN_LED_BLUE                   D3
#define PIN_LED_WHITE                  -1

/**
 * Pin configuration for LED_TYPE: WS2812B (only used if configured above)
 */

#define PIN_LED_SIGNAL                 D1
#define LED_COUNT                      60
