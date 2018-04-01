enum LEDType {
    RGB,
    RGBW
};

/*
 * Configuration
 */

#define DEBUG_LEVEL                             1

// Generate a random id with:
// $ echo -n "vibelight_"; head /dev/urandom | tr -dc A-Z0-9 | head -c 8; echo ""
#define VIBELIGHT_NODE_ID                       "vibelight_AAAABBBB"

#define WIFI_SSID                               ""
#define WIFI_PASSWORD                           ""

#define MQTT_CLIENTID                           VIBELIGHT_NODE_ID
#define MQTT_SERVER                             ""

// Use the SHA1 fingerprint of the server certificate (NOT the CA certificate) in the following format:
#define MQTT_SERVER_TLS_FINGERPRINT             "XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX"

#define MQTT_PORT                               8883
#define MQTT_USERNAME                           VIBELIGHT_NODE_ID
#define MQTT_PASSWORD                           ""

#define MQTT_CHANNEL_STATE                      "/vibelight/api/5/id/AAAABBBB/state/"
#define MQTT_CHANNEL_COMMAND                    "/vibelight/api/5/id/AAAABBBB/command/"

// Uncomment if on the board is a onboard LED
#define PIN_STATUSLED                           LED_BUILTIN

#define LED_TYPE                                RGB

/*
 * Define some optional offsets for color channels in the range 0..255
 * to trim some possible color inconsistency of the LED strip:
 */
#define LED_RED_OFFSET                          0
#define LED_GREEN_OFFSET                        0
#define LED_BLUE_OFFSET                         0
#define LED_WHITE_OFFSET                        0

#define PIN_LED_RED                             14
#define PIN_LED_GREEN                           13
#define PIN_LED_BLUE                            12
#define PIN_LED_WHITE                           11

#define CROSSFADE_ENABLED                       true
#define CROSSFADE_DELAY_MICROSECONDS            1500
#define CROSSFADE_STEPCOUNT                     256
