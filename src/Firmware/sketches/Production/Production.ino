#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"

#define FIRMWARE_VERSION  "5.0.0"

WiFiClientSecure secureWifiClient = WiFiClientSecure();
PubSubClient mqttClient = PubSubClient(secureWifiClient, MQTT_SERVER_TLS_FINGERPRINT);
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

bool stateOnOff;
int transitionAnimationDurationInMicroseconds;
uint8_t brightness;

// These color values are the original state values:
uint8_t originalRedValue = 0;
uint8_t originalGreenValue = 0;
uint8_t originalBlueValue = 0;
uint8_t originalWhiteValue = 0;

// These color values include color offset and brightness:
float currentRedValue = 0;
float currentGreenValue = 0;
float currentBlueValue = 0;
float currentWhiteValue = 0;

unsigned long lastTransitionAnimationUpdate = 0;

const uint8_t CROSSFADE_STEPCOUNT = 255;

uint8_t remainingCrossfadeSteps = 0;
int transitionAnimationStepDelayMicroseconds;

float valueChangePerCrossfadeStepRed = 0.0f;
float valueChangePerCrossfadeStepGreen = 0.0f;
float valueChangePerCrossfadeStepBlue = 0.0f;
float valueChangePerCrossfadeStepWhite = 0.0f;

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
    transitionAnimationDurationInMicroseconds = DEFAULT_TRANSITION_ANIMATION_DURATION_MICROSECONDS;
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

    showGivenColor(originalRedValue, originalGreenValue, originalBlueValue, originalWhiteValue, transitionAnimationDurationInMicroseconds);
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

        Serial.printf("onMessageReceivedCallback(): Message arrived on channel '%s': %s\n", topic, payloadMessage);

        if (updateValuesAccordingJsonMessage(payloadMessage))
        {
            publishState();
        }
        else
        {
            Serial.println("onMessageReceivedCallback(): The payload could not be parsed as JSON!");
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

        uint8_t newRedValue;
        uint8_t newGreenValue;
        uint8_t newBlueValue;
        uint8_t newWhiteValue;

        if (root.containsKey("color"))
        {
            newRedValue = constrainBetweenByte(root["color"]["r"]);
            newGreenValue = constrainBetweenByte(root["color"]["g"]);
            newBlueValue = constrainBetweenByte(root["color"]["b"]);
        }

        if (LED_TYPE == RGBW && root.containsKey("white_value"))
        {
            newWhiteValue = constrainBetweenByte(root["white_value"]);
        }

        if (root.containsKey("brightness"))
        {
            brightness = constrainBetweenByte(root["brightness"]);
        }

        if (root.containsKey("transition"))
        {
            // The maximum value for "transition" is 60 seconds (thus we always stay in __INT_MAX__)
            transitionAnimationDurationInMicroseconds = constrain(root["transition"], 0, 60) * 1000000;
        }
        else
        {
            transitionAnimationDurationInMicroseconds = DEFAULT_TRANSITION_ANIMATION_DURATION_MICROSECONDS;
        }

        #if DEBUG_LEVEL >= 1
            Serial.print(F("updateValuesAccordingJsonMessage(): The light was changed to: "));
            Serial.print(F("stateOnOff = "));
            Serial.print(stateOnOff);
            Serial.print(F(", brightness = "));
            Serial.print(brightness);
            Serial.print(F(", newRedValue = "));
            Serial.print(newRedValue);
            Serial.print(F(", newGreenValue = "));
            Serial.print(newGreenValue);
            Serial.print(F(", newBlueValue = "));
            Serial.print(newBlueValue);
            Serial.print(F(", newWhiteValue = "));
            Serial.print(newWhiteValue);
            Serial.print(F(", transitionAnimationDurationInMicroseconds = "));
            Serial.print(transitionAnimationDurationInMicroseconds);
            Serial.println();
        #endif

        originalRedValue = newRedValue;
        originalGreenValue = newGreenValue;
        originalBlueValue = newBlueValue;
        originalWhiteValue = (LED_TYPE == RGBW) ? newWhiteValue : 0;

        const uint8_t newRedValueWithOffset = constrainBetweenByte(originalRedValue + LED_RED_OFFSET);
        const uint8_t newGreenValueWithOffset = constrainBetweenByte(originalGreenValue + LED_GREEN_OFFSET);
        const uint8_t newBlueValueWithOffset = constrainBetweenByte(originalBlueValue + LED_BLUE_OFFSET);
        const uint8_t newWhiteValueWithOffset = (LED_TYPE == RGBW) ? constrainBetweenByte(originalWhiteValue + LED_WHITE_OFFSET) : 0;

        const uint8_t newRedValueWithBrightness = mapColorValueWithBrightness(newRedValueWithOffset, brightness);
        const uint8_t newGreenValueWithBrightness = mapColorValueWithBrightness(newGreenValueWithOffset, brightness);
        const uint8_t newBlueValueWithBrightness = mapColorValueWithBrightness(newBlueValueWithOffset, brightness);
        const uint8_t newWhiteValueWithBrightness = (LED_TYPE == RGBW) ? mapColorValueWithBrightness(newWhiteValueWithOffset, brightness) : 0;

        if (stateOnOff == true)
        {
            showGivenColor(newRedValueWithBrightness, newGreenValueWithBrightness, newBlueValueWithBrightness, newWhiteValueWithBrightness, transitionAnimationDurationInMicroseconds);
        }
        else
        {
            showGivenColor(0, 0, 0, 0, transitionAnimationDurationInMicroseconds);
        }
    }

    return wasSuccessfulParsed;
}

void showGivenColor(const float newRedValue, const float newGreenValue, const float newBlueValue, const float newWhiteValue, const long transitionAnimationDurationInMicroseconds)
{
    #if DEBUG_LEVEL >= 2
        Serial.print(F("showGivenColor(): newRedValue = "));
        Serial.print(newRedValue);
        Serial.print(F(", newGreenValue = "));
        Serial.print(newGreenValue);
        Serial.print(F(", newBlueValue = "));
        Serial.print(newBlueValue);
        Serial.print(F(", newWhiteValue = "));
        Serial.print(newWhiteValue);
        Serial.println();
    #endif

    if (transitionAnimationDurationInMicroseconds > 0)
    {
        startTransitionAnimation(newRedValue, newGreenValue, newBlueValue, newWhiteValue, transitionAnimationDurationInMicroseconds);
    }
    else
    {
        cancelRunningTransitionAnimation();
        showGivenColorImmediately(newRedValue, newGreenValue, newBlueValue, newWhiteValue);
    }
}

void startTransitionAnimation(const float newRedValue, const float newGreenValue, const float newBlueValue, const float newWhiteValue, const long transitionAnimationDurationInMicroseconds)
{
    valueChangePerCrossfadeStepRed = calculateValueChangePerStep(currentRedValue, newRedValue);
    valueChangePerCrossfadeStepGreen = calculateValueChangePerStep(currentGreenValue, newGreenValue);
    valueChangePerCrossfadeStepBlue = calculateValueChangePerStep(currentBlueValue, newBlueValue);
    valueChangePerCrossfadeStepWhite = calculateValueChangePerStep(currentWhiteValue, newWhiteValue);

    transitionAnimationStepDelayMicroseconds = transitionAnimationDurationInMicroseconds / CROSSFADE_STEPCOUNT;
    remainingCrossfadeSteps = CROSSFADE_STEPCOUNT;

    #if DEBUG_LEVEL >= 2
        Serial.print(F("startTransitionAnimation(): valueChangePerCrossfadeStepRed = "));
        Serial.print(valueChangePerCrossfadeStepRed);
        Serial.print(F(", valueChangePerCrossfadeStepGreen = "));
        Serial.print(valueChangePerCrossfadeStepGreen);
        Serial.print(F(", valueChangePerCrossfadeStepBlue = "));
        Serial.print(valueChangePerCrossfadeStepBlue);
        Serial.print(F(", valueChangePerCrossfadeStepWhite = "));
        Serial.print(valueChangePerCrossfadeStepWhite);
        Serial.print(F(", transitionAnimationStepDelayMicroseconds = "));
        Serial.print(transitionAnimationStepDelayMicroseconds);
        Serial.println();
    #endif
}

void cancelRunningTransitionAnimation()
{
    valueChangePerCrossfadeStepRed = 0.0f;
    valueChangePerCrossfadeStepGreen = 0.0f;
    valueChangePerCrossfadeStepBlue = 0.0f;
    valueChangePerCrossfadeStepWhite = 0.0f;

    remainingCrossfadeSteps = 0;
    transitionAnimationStepDelayMicroseconds = 0;
}

void showGivenColorImmediately(const float newRedValue, const float newGreenValue, const float newBlueValue, const float newWhiteValue)
{
    #if DEBUG_LEVEL >= 2
        Serial.print(F("showGivenColorImmediately(): newRedValue = "));
        Serial.print(newRedValue);
        Serial.print(F(", newGreenValue = "));
        Serial.print(newGreenValue);
        Serial.print(F(", newBlueValue = "));
        Serial.print(newBlueValue);
        Serial.print(F(", newWhiteValue = "));
        Serial.print(newWhiteValue);
        Serial.println();
    #endif

    currentRedValue = newRedValue;
    currentGreenValue = newGreenValue;
    currentBlueValue = newBlueValue;
    currentWhiteValue = newWhiteValue;

    analogWrite(PIN_LED_RED, round(currentRedValue));
    analogWrite(PIN_LED_GREEN, round(currentGreenValue));
    analogWrite(PIN_LED_BLUE, round(currentBlueValue));
    analogWrite(PIN_LED_WHITE, round(currentWhiteValue));
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

uint8_t constrainBetweenByte(const uint8_t valueToConstrain)
{
    return constrain(valueToConstrain, 0, 255);
}

uint8_t mapColorValueWithBrightness(const uint8_t colorValue, const uint8_t brigthnessValue)
{
    return map(colorValue, 0, 255, 0, brigthnessValue);
}

float calculateValueChangePerStep(const float startValue, const float endValue)
{
    return (endValue - startValue) / (float) CROSSFADE_STEPCOUNT;
}

void loop()
{
    connectMQTT();
    mqttClient.loop();
    updateTransitionAnimationIfNecessary();
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

        if (mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD) == true)
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

void updateTransitionAnimationIfNecessary()
{
    const bool animationStillRunning = remainingCrossfadeSteps > 0;
    const bool animationUpdateNecessary = (micros() - lastTransitionAnimationUpdate) > transitionAnimationStepDelayMicroseconds;

    if (animationStillRunning && animationUpdateNecessary)
    {
        const float newRedValue = currentRedValue + valueChangePerCrossfadeStepRed;
        const float newGreenValue = currentGreenValue + valueChangePerCrossfadeStepGreen;
        const float newBlueValue = currentBlueValue + valueChangePerCrossfadeStepBlue;
        const float newWhiteValue = currentWhiteValue + valueChangePerCrossfadeStepWhite;
        showGivenColorImmediately(newRedValue, newGreenValue, newBlueValue, newWhiteValue);

        // TODO: check if currentvalue == newvalue -> skip remaining steps

        lastTransitionAnimationUpdate = micros();
        remainingCrossfadeSteps--;
    }
}
