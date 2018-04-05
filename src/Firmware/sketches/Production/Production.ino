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

/*
 * These color values are the original state values:
 */
uint8_t originalRedValue = 0;
uint8_t originalGreenValue = 0;
uint8_t originalBlueValue = 0;
uint8_t originalWhiteValue = 0;

/*
 * These color values include color offset and brightness:
 */
uint8_t currentRedValue = 0;
uint8_t currentGreenValue = 0;
uint8_t currentBlueValue = 0;
uint8_t currentWhiteValue = 0;

/*
 * These color values are only used for transition animation
 * and represent transition start value:
 */
uint8_t transitionAnimationStartRedValue = 0;
uint8_t transitionAnimationStartGreenValue = 0;
uint8_t transitionAnimationStartBlueValue = 0;
uint8_t transitionAnimationStartWhiteValue = 0;

/*
 * These color values are only used for transition animation
 * and represent transition end value:
 */
uint8_t transitionAnimationEndRedValue = 0;
uint8_t transitionAnimationEndGreenValue = 0;
uint8_t transitionAnimationEndBlueValue = 0;
uint8_t transitionAnimationEndWhiteValue = 0;

bool transitionAnimationRunning = false;

unsigned long lastTransitionAnimationUpdate = 0;
int transitionAnimationStepDelayMicroseconds;

const uint8_t TRANSITION_ANIMATION_STEPCOUNT = 255;
uint8_t remainingTransitionAnimationSteps = 0;

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

    setupWifi();
    setupLEDs();
    setupMQTT();
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
        Serial.print(F("setupLEDs():"));
        Serial.print(F(" originalRedValue = "));
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
    {"brightness": 120, "color": {"r": 255, "g": 0, "b": 0}, "white_value": 255, "transition": 5, "state": "ON"}

    Example payload (RGB):
    {"brightness": 120, "color": {"r": 255, "g": 0, "b": 0}, "transition": 5, "state": "ON"}
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
            Serial.print(F(", redValue = "));
            Serial.print(redValue);
            Serial.print(F(", greenValue = "));
            Serial.print(greenValue);
            Serial.print(F(", blueValue = "));
            Serial.print(blueValue);
            Serial.print(F(", whiteValue = "));
            Serial.print(whiteValue);
            Serial.print(F(", transitionAnimationDurationInMicroseconds = "));
            Serial.print(transitionAnimationDurationInMicroseconds);
            Serial.println();
        #endif

        originalRedValue = redValue;
        originalGreenValue = greenValue;
        originalBlueValue = blueValue;
        originalWhiteValue = (LED_TYPE == RGBW) ? whiteValue : 0;

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

uint8_t constrainBetweenByte(const uint8_t valueToConstrain)
{
    return constrain(valueToConstrain, 0, 255);
}

uint8_t mapColorValueWithBrightness(const uint8_t colorValue, const uint8_t brightnessValue)
{
    return map(colorValue, 0, 255, 0, brightnessValue);
}

void showGivenColor(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue, const long transitionAnimationDurationInMicroseconds)
{
    // TODO: check if new != current

    #if DEBUG_LEVEL >= 2
        Serial.print(F("showGivenColor():"));
        Serial.print(F(" redValue = "));
        Serial.print(redValue);
        Serial.print(F(", greenValue = "));
        Serial.print(greenValue);
        Serial.print(F(", blueValue = "));
        Serial.print(blueValue);
        Serial.print(F(", whiteValue = "));
        Serial.print(whiteValue);
        Serial.println();
    #endif

    if (transitionAnimationDurationInMicroseconds > 0)
    {
        startTransitionAnimation(redValue, greenValue, blueValue, whiteValue, transitionAnimationDurationInMicroseconds);
    }
    else
    {
        cancelTransitionAnimation();
        showGivenColorImmediately(redValue, greenValue, blueValue, whiteValue);
    }
}

void startTransitionAnimation(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue, const long transitionAnimationDurationInMicroseconds)
{
    transitionAnimationStartRedValue = currentRedValue;
    transitionAnimationStartGreenValue = currentGreenValue;
    transitionAnimationStartBlueValue = currentBlueValue;
    transitionAnimationStartWhiteValue = currentWhiteValue;

    transitionAnimationEndRedValue = redValue;
    transitionAnimationEndGreenValue = greenValue;
    transitionAnimationEndBlueValue = blueValue;
    transitionAnimationEndWhiteValue = whiteValue;

    transitionAnimationRunning = true;
    transitionAnimationStepDelayMicroseconds = transitionAnimationDurationInMicroseconds / TRANSITION_ANIMATION_STEPCOUNT;
    remainingTransitionAnimationSteps = TRANSITION_ANIMATION_STEPCOUNT;

    #if DEBUG_LEVEL >= 2
        Serial.print(F("startTransitionAnimation():"));
        Serial.print(F(" transitionAnimationStartRedValue = "));
        Serial.print(transitionAnimationStartRedValue);
        Serial.print(F(", transitionAnimationStartGreenValue = "));
        Serial.print(transitionAnimationStartGreenValue);
        Serial.print(F(", transitionAnimationStartBlueValue = "));
        Serial.print(transitionAnimationStartBlueValue);
        Serial.print(F(", transitionAnimationStartWhiteValue = "));
        Serial.print(transitionAnimationStartWhiteValue);
        Serial.print(F(" transitionAnimationEndRedValue = "));
        Serial.print(transitionAnimationEndRedValue);
        Serial.print(F(", transitionAnimationEndGreenValue = "));
        Serial.print(transitionAnimationEndGreenValue);
        Serial.print(F(", transitionAnimationEndBlueValue = "));
        Serial.print(transitionAnimationEndBlueValue);
        Serial.print(F(", transitionAnimationEndWhiteValue = "));
        Serial.print(transitionAnimationEndWhiteValue);
        Serial.print(F(" transitionAnimationStepDelayMicroseconds = "));
        Serial.print(transitionAnimationStepDelayMicroseconds);
        Serial.println();
    #endif
}

void cancelTransitionAnimation()
{
    transitionAnimationStartRedValue = 0;
    transitionAnimationStartGreenValue = 0;
    transitionAnimationStartBlueValue = 0;
    transitionAnimationStartWhiteValue = 0;

    transitionAnimationEndRedValue = 0;
    transitionAnimationEndGreenValue = 0;
    transitionAnimationEndBlueValue = 0;
    transitionAnimationEndWhiteValue = 0;

    transitionAnimationRunning = false;
    transitionAnimationStepDelayMicroseconds = 0;
    remainingTransitionAnimationSteps = 0;
}

void showGivenColorImmediately(const uint8_t redValue, const uint8_t greenValue, const uint8_t blueValue, const uint8_t whiteValue)
{
    #if DEBUG_LEVEL >= 2
        Serial.print(F("showGivenColorImmediately():"));
        Serial.print(F(" redValue = "));
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
    // Check if the animation must be updated
    if ((micros() - lastTransitionAnimationUpdate) > transitionAnimationStepDelayMicroseconds)
    {
        // TODO: fix last step
        if (transitionAnimationRunning && remainingTransitionAnimationSteps > 0)
        {
            const uint8_t redValue = getColorValueForStep(remainingTransitionAnimationSteps, transitionAnimationStartRedValue, transitionAnimationEndRedValue);
            const uint8_t greenValue = getColorValueForStep(remainingTransitionAnimationSteps, transitionAnimationStartGreenValue, transitionAnimationEndGreenValue);
            const uint8_t blueValue = getColorValueForStep(remainingTransitionAnimationSteps, transitionAnimationStartBlueValue, transitionAnimationEndBlueValue);
            const uint8_t whiteValue = getColorValueForStep(remainingTransitionAnimationSteps, transitionAnimationStartWhiteValue, transitionAnimationEndWhiteValue);
            showGivenColorImmediately(redValue, greenValue, blueValue, whiteValue);

            lastTransitionAnimationUpdate = micros();
            remainingTransitionAnimationSteps--; 
        }
        else
        {
            transitionAnimationRunning = false;
            remainingTransitionAnimationSteps = 0;
        }
    }
}

uint8_t getColorValueForStep(const uint8_t remainingSteps, const uint8_t startColorValue, const uint8_t endColorValue)
{
    // For safety the value is contained before writing to uint8_t, to be sure no signed values are written
    const uint8_t unsignedColorValueForStep = constrain((endColorValue - startColorValue) - remainingSteps, 0, 255);
    return unsignedColorValueForStep;
}
