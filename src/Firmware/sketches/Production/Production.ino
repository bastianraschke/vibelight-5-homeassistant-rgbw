#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"

#define FIRMWARE_VERSION                        "5.0.0"

WiFiClientSecure secureWifiClient = WiFiClientSecure();
PubSubClient MQTTClient = PubSubClient(secureWifiClient);

bool stateOnOff;
bool transitionEffectEnabled;
uint8_t brightness; 

// These color values are the original state values:
uint8_t originalRedValue = 0;
uint8_t originalGreenValue = 0;
uint8_t originalBlueValue = 0;
uint8_t originalWhiteValue = 0;

// These color values include color offset and brightness:
uint8_t currentRedValue = 0;
uint8_t currentGreenValue = 0; 
uint8_t currentBlueValue = 0;
uint8_t currentWhiteValue = 0;

/*
 * Setup
 */

void setup()
{
    Serial.begin(115200);
    delay(250);

    char buffer[64] = {0};
    sprintf(buffer, "setup(): The node '%s' was powered up.", MQTT_CLIENTID);
    Serial.println();
    Serial.println(buffer);

    setupLEDs();
    setupWifi();
    setupMQTT();
}

void setupLEDs()
{
    Serial.println("setupLEDs(): Setup LEDs...");

    #ifdef PIN_STATUSLED
        pinMode(PIN_STATUSLED, OUTPUT);
    #endif

    // Set initial values for LED
    stateOnOff = true;
    transitionEffectEnabled = true;
    brightness = 255;

    // For RGBW LED type show only the native white LEDs
    if (LED_TYPE == RGBW)
    {
        originalRedValue = 0;
        originalGreenValue = 0;
        originalBlueValue = 0;
        originalWhiteValue = 255;
    }
    else
    {
        originalRedValue = 255;
        originalGreenValue = 255;
        originalBlueValue = 255;
        originalWhiteValue = 0;
    }

    #if DEBUG_LEVEL >= 2
        Serial.print(F("setupLEDs(): originalRedValue = "));
        Serial.print(originalRedValue);
        Serial.print(F(", originalGreenValue = "));
        Serial.print(originalGreenValue);
        Serial.print(F(", originalBlueValue = "));
        Serial.print(originalBlueValue);
        Serial.print(F(", originalWhiteValue = "));
        Serial.print(originalWhiteValue);
        Serial.println();
    #endif

    showGivenColor(originalRedValue, originalGreenValue, originalBlueValue, originalWhiteValue, transitionEffectEnabled);
}

void setupWifi()
{
    Serial.printf("setupWifi(): Connecting to to Wi-Fi access point '%s'...\n", WIFI_SSID);

    // Do not store Wi-Fi config in SDK flash area
    WiFi.persistent(false);

    // Disable auto Wi-Fi access point mode
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        // Blink 2 times when connecting
        blinkStatusLED(2);

        delay(500);
        Serial.println(F("setupWifi(): Connecting..."));
    }

    Serial.print(F("setupWifi(): Connected to Wi-Fi access point. Obtained IP address: "));
    Serial.println(WiFi.localIP());
}

void blinkStatusLED(const int times)
{
    #ifdef PIN_STATUSLED
        for (int i = 0; i < times; i++)
        {
            // Enable LED
            digitalWrite(PIN_STATUSLED, LOW);
            delay(100);

            // Disable LED
            digitalWrite(PIN_STATUSLED, HIGH);
            delay(100);
        }
    #endif
}

void setupMQTT()
{
    MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
    MQTTClient.setCallback(onMessageReceivedCallback);
}

void onMessageReceivedCallback(char* topic, byte* payload, unsigned int length)
{
    if (!topic || !payload)
    {
        Serial.println("onMessageReceivedCallback(): Invalid argument (nullpointer) given!");
    }
    else
    {
        Serial.printf("onMessageReceivedCallback(): Message arrived on channel: %s\n", topic);

        char message[length + 1];

        for (int i = 0; i < length; i++)
        {
            message[i] = (char)payload[i];
        }

        message[length] = '\0';

        Serial.printf("onMessageReceivedCallback(): Message arrived on channel '%s':\n%s\n", topic, message);

        // TODO: Handle message



    }
}

/*
Example payload (RGBW):

    {
      "brightness": 120,
      "color": {
        "r": 255,
        "g": 100,
        "b": 100
      },
      "white_value": 255,
      "state": "ON"
    }
*/
// void onLightChangedPacketReceived(asbPacket &canPacket)
// {
//     stateOnOff = canPacket.data[1];
//     brightness = constrainBetweenByte(canPacket.data[2]);

//     const uint8_t redValue = constrainBetweenByte(canPacket.data[3]);
//     const uint8_t greenValue = constrainBetweenByte(canPacket.data[4]);
//     const uint8_t blueValue = constrainBetweenByte(canPacket.data[5]);
//     const uint8_t whiteValue = constrainBetweenByte(canPacket.data[6]);

//     transitionEffectEnabled = (canPacket.data[7] == 0x01);

//     #if DEBUG_LEVEL >= 1
//         Serial.print(F("onLightChangedPacketReceived(): The light was changed to: "));
//         Serial.print(F("stateOnOff = "));
//         Serial.print(stateOnOff);
//         Serial.print(F(", brightness = "));
//         Serial.print(brightness);
//         Serial.print(F(", redValue = "));
//         Serial.print(redValue);
//         Serial.print(F(", greenValue = "));
//         Serial.print(greenValue);
//         Serial.print(F(", blueValue = "));
//         Serial.print(blueValue);
//         Serial.print(F(", whiteValue = "));
//         Serial.print(whiteValue);
//         Serial.print(F(", transitionEffectEnabled = "));
//         Serial.print(transitionEffectEnabled);
//         Serial.println();
//     #endif

//     originalRedValue = redValue;
//     originalGreenValue = greenValue;
//     originalBlueValue = blueValue;
//     originalWhiteValue = (LED_TYPE == RGBW) ? whiteValue : 0;

//     const uint8_t redValueWithOffset = constrainBetweenByte(originalRedValue + LED_RED_OFFSET);
//     const uint8_t greenValueWithOffset = constrainBetweenByte(originalGreenValue + LED_GREEN_OFFSET);
//     const uint8_t blueValueWithOffset = constrainBetweenByte(originalBlueValue + LED_BLUE_OFFSET);
//     const uint8_t whiteValueWithOffset = (LED_TYPE == RGBW) ? constrainBetweenByte(originalWhiteValue + LED_WHITE_OFFSET) : 0;

//     const uint8_t redValueWithBrightness = mapColorValueWithBrightness(redValueWithOffset, brightness);
//     const uint8_t greenValueWithBrightness = mapColorValueWithBrightness(greenValueWithOffset, brightness);
//     const uint8_t blueValueWithBrightness = mapColorValueWithBrightness(blueValueWithOffset, brightness);
//     const uint8_t whiteValueWithBrightness = (LED_TYPE == RGBW) ? mapColorValueWithBrightness(whiteValueWithOffset, brightness) : 0;

//     if (stateOnOff == true)
//     {
//         showGivenColor(redValueWithBrightness, greenValueWithBrightness, blueValueWithBrightness, whiteValueWithBrightness, transitionEffectEnabled);
//     }
//     else
//     {
//         showGivenColor(0, 0, 0, 0, transitionEffectEnabled);
//     }
// }

void showGivenColor(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue, const bool transitionEffectEnabled)
{
    #if DEBUG_LEVEL >= 2
        Serial.print(F("showGivenColor(): redValue = "));
        Serial.print(redValue);
        Serial.print(F(", greenValue = "));
        Serial.print(greenValue);
        Serial.print(F(", blueValue = "));
        Serial.print(blueValue);
        Serial.print(F(", whiteValue = "));
        Serial.print(whiteValue);
        Serial.println();
    #endif

    if (CROSSFADE_ENABLED && transitionEffectEnabled)
    {
        showGivenColorWithTransition(redValue, greenValue, blueValue, whiteValue);
    }
    else
    {
        showGivenColorImmediately(redValue, greenValue, blueValue, whiteValue);
    }
}

void showGivenColorWithTransition(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{
    // Calculate step value to get from current shown color to new color
    const float valueChangePerStepRed = calculateValueChangePerStep(currentRedValue, redValue);
    const float valueChangePerStepGreen = calculateValueChangePerStep(currentGreenValue, greenValue);
    const float valueChangePerStepBlue = calculateValueChangePerStep(currentBlueValue, blueValue);
    const float valueChangePerStepWhite = calculateValueChangePerStep(currentWhiteValue, whiteValue);

    #if DEBUG_LEVEL >= 2
        Serial.print(F("showGivenColorWithTransition(): valueChangePerStepRed = "));
        Serial.print(valueChangePerStepRed);
        Serial.print(F(", valueChangePerStepGreen = "));
        Serial.print(valueChangePerStepGreen);
        Serial.print(F(", valueChangePerStepBlue = "));
        Serial.print(valueChangePerStepBlue);
        Serial.print(F(", valueChangePerStepWhite = "));
        Serial.print(valueChangePerStepWhite);
        Serial.println();
    #endif

    // Start temporary color variable with current color value
    float tempRedValue = currentRedValue;
    float tempGreenValue = currentGreenValue; 
    float tempBlueValue = currentBlueValue;
    float tempWhiteValue = currentWhiteValue;

    // For N steps, add the step value to the temporary color variable to have new current color value 
    for (int i = 0; i < CROSSFADE_STEPCOUNT; i++)
    {
        tempRedValue = tempRedValue + valueChangePerStepRed;
        tempGreenValue = tempGreenValue + valueChangePerStepGreen;
        tempBlueValue = tempBlueValue + valueChangePerStepBlue;
        tempWhiteValue = tempWhiteValue + valueChangePerStepWhite;

        showGivenColorImmediately(
            round(tempRedValue),
            round(tempGreenValue),
            round(tempBlueValue),
            round(tempWhiteValue)
        );

        delayMicroseconds(CROSSFADE_DELAY_MICROSECONDS);
    }
}

void showGivenColorImmediately(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{
    #if DEBUG_LEVEL >= 2
        Serial.print(F("showGivenColorImmediately(): redValue = "));
        Serial.print(redValue);
        Serial.print(F(", greenValue = "));
        Serial.print(greenValue);
        Serial.print(F(", blueValue = "));
        Serial.print(blueValue);
        Serial.print(F(", whiteValue = "));
        Serial.print(whiteValue);
        Serial.println();
    #endif

    currentRedValue = redValue;
    currentGreenValue = greenValue;
    currentBlueValue = blueValue;
    currentWhiteValue = whiteValue;

    analogWrite(PIN_LED_RED, currentRedValue);
    analogWrite(PIN_LED_GREEN, currentGreenValue);
    analogWrite(PIN_LED_BLUE, currentBlueValue);
    analogWrite(PIN_LED_WHITE, currentWhiteValue);
}

uint8_t constrainBetweenByte(const uint8_t valueToConstrain)
{
    return constrain(valueToConstrain, 0, 255);
}

uint8_t mapColorValueWithBrightness(const uint8_t colorValue, const uint8_t brigthnessValue)
{
    return map(colorValue, 0, 255, 0, brigthnessValue);
}

float calculateValueChangePerStep(const uint8_t startValue, const uint8_t endValue)
{
    return ((float) (endValue - startValue)) / ((float) CROSSFADE_STEPCOUNT);
}

void loop()
{
    connectMQTT();
    MQTTClient.loop();
}

void connectMQTT()
{
    while (MQTTClient.connected() == false)
    {
        Serial.println("connectMQTT(): Connecting...");

        if (MQTTClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_SERVER_TLS_FINGERPRINT) == true)
        {
            Serial.println("connectMQTT(): Connected to MQTT broker.");

            // (Re)subscribe on topics
            MQTTClient.subscribe(MQTT_CHANNEL_COMMAND);
        }
        else
        {
            Serial.printf("connectMQTT(): Connection failed! Error code: %i\n", MQTTClient.state());

            // Blink 3 times for indication of failed MQTT connection
            blinkStatusLED(3);

            Serial.println("connectMQTT(): Try again in 1 second...");
            delay(1000);
        }
    }
}
