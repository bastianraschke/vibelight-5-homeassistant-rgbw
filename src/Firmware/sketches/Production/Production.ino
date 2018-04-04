#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"

#define FIRMWARE_VERSION  "5.0.0"

WiFiClientSecure secureWifiClient = WiFiClientSecure();
PubSubClient mqttClient = PubSubClient(secureWifiClient);
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

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
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(onMessageReceivedCallback);
}

void onMessageReceivedCallback(char* topic, byte* payload, unsigned int length)
{
    if (!topic || !payload)
    {
        Serial.println("onMessageReceivedCallback(): Invalid argument (nullpointer) given!");
    }
    else
    {
        char payloadMessage[length + 1];

        for (int i = 0; i < length; i++)
        {
            payloadMessage[i] = (char) payload[i];
        }

        payloadMessage[length] = '\0';

        Serial.printf("onMessageReceivedCallback(): Message arrived on channel '%s':\n%s\n", topic, payloadMessage);

        if (updateValuesAccordingJsonMessage(payloadMessage))
        {
            publishState();
            blinkStatusLED(1);
        }
        else
        {
            Serial.println("onMessageReceivedCallback(): The payload could not be parsed as JSON!");
            blinkStatusLED(2);
        }
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
      "transition": 5,
      "state": "ON"
    }
*/
bool updateValuesAccordingJsonMessage(char* jsonPayload)
{
    bool wasSuccessfulParsed = true;

    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(jsonPayload);

    if (!root.success())
    {
        wasSuccessfulParsed = false;
    }
    else
    {
        if (root.containsKey("state"))
        {
            if (strcmp(root["state"], "ON") == 0)
            {
                stateOnOff = true;
            }
            else if (strcmp(root["state"], "OFF") == 0)
            {
                stateOnOff = false;
            }
        }

        uint8_t redValue;
        uint8_t greenValue;
        uint8_t blueValue;
        uint8_t whiteValue;

        if (root.containsKey("color"))
        {
            redValue = constrainBetweenByte(root["color"]["r"]);
            greenValue = constrainBetweenByte(root["color"]["g"]);
            blueValue = constrainBetweenByte(root["color"]["b"]);
        }

        if (LED_TYPE == RGBW && root.containsKey("white_value"))
        {
            whiteValue = constrainBetweenByte(root["white_value"]);
        }

        if (root.containsKey("brightness"))
        {
            brightness = constrainBetweenByte(root["brightness"]);
        }

        // TODO: Get from payload
        transitionEffectEnabled = true;

    // if (root.containsKey("transition")) {
    //   transitionTime = root["transition"];
    // }
    // else {
    //   transitionTime = 0;
    // }

        #if DEBUG_LEVEL >= 1
            Serial.print(F("updateValuesAccordingJsonMessage(): The light was changed to: "));
            Serial.print(F("stateOnOff = "));
            Serial.print(stateOnOff);
            Serial.print(F(", brightness = "));
            Serial.print(brightness);
            Serial.print(F(", redValue = "));
            Serial.print(redValue);
            Serial.print(F(", greenValue = "));
            Serial.print(greenValue);
            Serial.print(F(", blueValue = "));
            Serial.print(blueValue);
            Serial.print(F(", whiteValue = "));
            Serial.print(whiteValue);
            Serial.print(F(", transitionEffectEnabled = "));
            Serial.print(transitionEffectEnabled);
            Serial.println();
        #endif

        originalRedValue = redValue;
        originalGreenValue = greenValue;
        originalBlueValue = blueValue;
        originalWhiteValue = (LED_TYPE == RGBW) ? whiteValue : 0;

        const uint8_t redValueWithOffset = constrainBetweenByte(originalRedValue + LED_RED_OFFSET);
        const uint8_t greenValueWithOffset = constrainBetweenByte(originalGreenValue + LED_GREEN_OFFSET);
        const uint8_t blueValueWithOffset = constrainBetweenByte(originalBlueValue + LED_BLUE_OFFSET);
        const uint8_t whiteValueWithOffset = (LED_TYPE == RGBW) ? constrainBetweenByte(originalWhiteValue + LED_WHITE_OFFSET) : 0;

        const uint8_t redValueWithBrightness = mapColorValueWithBrightness(redValueWithOffset, brightness);
        const uint8_t greenValueWithBrightness = mapColorValueWithBrightness(greenValueWithOffset, brightness);
        const uint8_t blueValueWithBrightness = mapColorValueWithBrightness(blueValueWithOffset, brightness);
        const uint8_t whiteValueWithBrightness = (LED_TYPE == RGBW) ? mapColorValueWithBrightness(whiteValueWithOffset, brightness) : 0;

        if (stateOnOff == true)
        {
            showGivenColor(redValueWithBrightness, greenValueWithBrightness, blueValueWithBrightness, whiteValueWithBrightness, transitionEffectEnabled);
        }
        else
        {
            showGivenColor(0, 0, 0, 0, transitionEffectEnabled);
        }
    }

    return wasSuccessfulParsed;
}

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

        // TODO: Do not delay to keep connection alive
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

void publishState()
{
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["state"] = (stateOnOff == true) ? "ON" : "OFF";

    JsonObject& color = root.createNestedObject("color");
    color["r"] = originalRedValue;
    color["g"] = originalGreenValue;
    color["b"] = originalWhiteValue;

    if (LED_TYPE == RGBW)
    {
        root["white_value"] = originalWhiteValue;
    }

    root["brightness"] = brightness;

    char buffer[root.measureLength() + 1];
    root.printTo(buffer, sizeof(buffer));

    mqttClient.publish(MQTT_CHANNEL_STATE, buffer, true);
}

void loop()
{
    connectMQTT();
    mqttClient.loop();
}

void connectMQTT()
{
    if (mqttClient.connected() == true)
    {
        return ;
    }

    Serial.printf("connectMQTT(): Connecting to MQTT broker '%s:%i'...\n", MQTT_SERVER, MQTT_PORT);

    while (mqttClient.connected() == false)
    {
        Serial.println("connectMQTT(): Connecting...");

        if (mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_SERVER_TLS_FINGERPRINT) == true)
        {
            Serial.println("connectMQTT(): Connected to MQTT broker.");

            // (Re)subscribe on topics
            mqttClient.subscribe(MQTT_CHANNEL_COMMAND);

            // Initially publish current state
            publishState();
        }
        else
        {
            Serial.printf("connectMQTT(): Connection failed with error code %i. Try again...\n", mqttClient.state());
            blinkStatusLED(3);
            delay(500);
        }
    }
}
