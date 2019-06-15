#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"

#define FIRMWARE_VERSION  "5.2.0"

uint8_t constrainBetweenByte(const int valueToConstrain) {
    return constrain(valueToConstrain, 0, 255);
}

#define ANIMATION_STEP_COUNT 256

class LEDStrip {
    public:
        Effect supportedEffects[0] = {};

        void setupIsWhiteSupported(const bool isWhiteSupported) {
            this->isWhiteSupported = isWhiteSupported;
        }

        void setupColorCorrectionOffsets(const uint8_t redOffset, const uint8_t greenOffset, const uint8_t blueOffset, const uint8_t whiteOffset) {
            this->colorCorrectionOffset = Color {
                constrainBetweenByte(redOffset),
                constrainBetweenByte(greenOffset),
                constrainBetweenByte(blueOffset),
                isWhiteSupported ? constrainBetweenByte(whiteOffset) : 0
            };
        }

        // TODO: Merge setup methods`to avoid order related bugs?
        virtual void setup() = 0;

        void setState(const bool state) {
            this->state = state;

            // TODO set black
        }

        void setColor(const Color color) {
            originalColor = Color {
                constrainBetweenByte(color.red),
                constrainBetweenByte(color.green),
                constrainBetweenByte(color.blue),
                isWhiteSupported ? constrainBetweenByte(color.white) : 0
            };

            updateDesiredColor();
        }

        void setBrightness(const uint8_t brightness) {
            this->brightness = brightness;
            updateDesiredColor();
        }

        void setTransitionDuration(const uint32_t transitionDuration) {
            this->transitionDuration = transitionDuration;
        }

        void setEffect(const Effect effect) {
            if (isEffectSupported(effect)) {
                this->effect = effect;
            }
        }

        void show() {
            if (effect != NONE) {
                showEffect();
            } else {
                showColor();
            }
        }

        void loop() {
            if (animationRunning) {
                if ((micros() - lastAnimationUpdate) > animationStepDelay) {
                    lastAnimationUpdate = micros();
                    onAnimationUpdate();

                    animationStepIndex = (animationStepIndex + 1) % ANIMATION_STEP_COUNT;

                    const bool animationCycleFinished = (animationStepIndex == 0);

                    // Reset step index and effect direction if animation cycle finished
                    if (animationCycleFinished) {
                        effectDirection = (effectDirection == LEFT) ? RIGHT : LEFT;
                    }

                    // Stop animation if it is not running for infinity
                    if (animationCycleFinished && !infinityAnimation) {
                        resetAnimationStates();
                    }
                }
            } else {
                resetAnimationStates();
            }
        }

        bool getIsWhiteSupported() {
            return isWhiteSupported;
        }

        bool getState() {
            return state;
        }

        Color getColor() {
            return originalColor;
        }

        uint8_t getBrightness() {
            return brightness;
        }

        uint32_t getTransitionDuration() {
            return transitionDuration;
        }

        Effect getEffect() {
            return effect;
        }

    protected:

        bool isWhiteSupported = false;
        Color colorCorrectionOffset = {0, 0, 0, 0};

        /*
         * States
         */

        bool state = false;
        uint8_t brightness = 0;
        uint32_t transitionDuration = LED_TRANSITION_DURATION;
        Effect effect = NONE;

        /*
         * Color states
         */

        Color originalColor = {0, 0, 0, 0};

        Color previousColor = {0, 0, 0, 0};
        Color desiredColor = {0, 0, 0, 0};

        /*
         * Animation states
         */

        bool animationRunning = false;
        bool infinityAnimation = false;

        unsigned long lastAnimationUpdate = 0;

        EffectDirection effectDirection = LEFT;

        uint8_t animationStepIndex = 0;
        uint32_t animationStepDelay = 10000;

        Color calculateAnimationStateColor(const uint8_t animationIndex, const Color startColor, const Color endColor) {
            return Color {
                calculateAnimationStateValue(animationStepIndex, startColor.red, endColor.red),
                calculateAnimationStateValue(animationStepIndex, startColor.green, endColor.green),
                calculateAnimationStateValue(animationStepIndex, startColor.blue, endColor.blue),
                calculateAnimationStateValue(animationStepIndex, startColor.white, endColor.white)
            };
        }

        uint8_t calculateAnimationStateValue(const uint8_t animationIndex, const uint8_t startValue, const uint8_t endValue) {
            uint8_t animationStateValue;

            // Prevent division-by-zero
            if (startValue == endValue) {
                animationStateValue = startValue;
            } else {
                animationStateValue = map(animationIndex, 0, 255, startValue, endValue);
            }

            return animationStateValue;
        }

        Color calculateCurrentWheelColor(uint8_t wheelPosition) {
            Color currentWheelColor;
            wheelPosition = 255 - wheelPosition;

            if (wheelPosition < 85) {
                currentWheelColor = Color {255 - wheelPosition * 3, 0, wheelPosition * 3, 0};
            } else if (wheelPosition < 170) {
                wheelPosition -= 85;
                currentWheelColor = Color {0, wheelPosition * 3, 255 - wheelPosition * 3, 0};
            } else {
                wheelPosition -= 170;
                currentWheelColor = Color {wheelPosition * 3, 255 - wheelPosition * 3, 0, 0};
            }

            return currentWheelColor;
        }

    private:

        bool isEffectSupported(const Effect effect) {
            bool isEffectSupported = false;

            // TODO: Check if it works
            for (int i = 0; i < sizeof(supportedEffects) / sizeof(Effect); i++) {
                if (supportedEffects[i] == effect) {
                    isEffectSupported = true;
                    break;
                }
            }

            return isEffectSupported;
        }

        void updateDesiredColor() {
            const uint8_t ofsettedRedColor = constrainBetweenByte(originalColor.red + colorCorrectionOffset.red);
            const uint8_t ofsettedGreenColor = constrainBetweenByte(originalColor.green + colorCorrectionOffset.green);
            const uint8_t ofsettedBlueColor = constrainBetweenByte(originalColor.blue + colorCorrectionOffset.blue);
            const uint8_t ofsettedWhiteColor = constrainBetweenByte(originalColor.white + colorCorrectionOffset.white);

            desiredColor = Color {
                calculateColorValueWithBrightness(ofsettedRedColor),
                calculateColorValueWithBrightness(ofsettedGreenColor),
                calculateColorValueWithBrightness(ofsettedBlueColor),
                calculateColorValueWithBrightness(ofsettedWhiteColor)
            };
        }

        uint8_t calculateColorValueWithBrightness(const uint8_t colorValue) {
            const uint8_t maximumBrightnessLimit = map(LED_MAX_BRIGHTNESS, 0, 100, 0, 255);
            const uint8_t limitedBrightnessValue = map(brightness, 0, 255, 0, maximumBrightnessLimit);
            return map(colorValue, 0, 255, 0, limitedBrightnessValue);
        }

        void showColor() {
            this->effect = NONE;
            showEffect();
        }

        void showEffect() {
            // Always reset animation state when new effect is set to be sure it starts always the same
            resetAnimationStates();

            switch(effect) {
                case RAINBOW:
                    animationStepDelay = 25000;
                    infinityAnimation = true;
                    break;
                case RAINBOW_CYCLE:
                    animationStepDelay = 12000;
                    infinityAnimation = true;
                    break;
                case LASERSCANNER:
                    animationStepDelay = 2000;
                    infinityAnimation = true;
                    break;
                case NONE:
                    animationStepDelay = transitionDuration / ANIMATION_STEP_COUNT;
                    infinityAnimation = false;
                    break;
            }

            animationRunning = true;
        }

        void resetAnimationStates() {
            animationRunning = false;
            lastAnimationUpdate = 0;

            effectDirection = LEFT;

            animationStepIndex = 0;
            animationStepDelay = 0;
        }

        void onAnimationUpdate() {
            switch(effect) {
                case RAINBOW:
                    updateRainbowAnimation();
                    break;
                case RAINBOW_CYCLE:
                    updateRainbowCycleAnimation();
                    break;
                case LASERSCANNER:
                    updateLaserscannerAnimation();
                    break;
                case NONE:
                    updateColor();
                    break;
            }
        }

        virtual void updateRainbowAnimation() = 0;
        virtual void updateRainbowCycleAnimation() = 0;
        virtual void updateLaserscannerAnimation() = 0;
        virtual void updateColor() = 0;
};

class WS2812BStrip : public LEDStrip {

    public:
        Effect supportedEffects[3] = {RAINBOW, RAINBOW_CYCLE, LASERSCANNER};

        WS2812BStrip(const uint8_t neopixelPin, const uint8_t neopixelCount) {
            neopixelStrip = Adafruit_NeoPixel(neopixelCount, neopixelPin, NEO_GRB + NEO_KHZ800);
        }

        virtual void setup() {
            neopixelStrip.begin();
        }

    private:
        Adafruit_NeoPixel neopixelStrip;

        virtual void updateRainbowAnimation() {
            /*
            for (int i = 0; i < neopixelStrip.numPixels(); i++) {
                const Color currentWheelColor = calculateCurrentWheelColor((i + animationStepIndex) & 255)
                neopixelStrip.setPixelColor(i, currentWheelColor.red, currentWheelColor.green, currentWheelColor.blue, currentWheelColor.white);
            }

            neopixelStrip.show();
            */
        }

        virtual void updateRainbowCycleAnimation() {
            /*
            for (int i = 0; i < neopixelStrip.numPixels(); i++) {
                const Color currentWheelColor = calculateCurrentWheelColor(((i * 256 / neopixelStrip.numPixels()) + animationStepIndex) & 255)
                neopixelStrip.setPixelColor(i, currentWheelColor.red, currentWheelColor.green, currentWheelColor.blue, currentWheelColor.white);
            }

            neopixelStrip.show();
            */
        }

        virtual void updateLaserscannerAnimation() {
            /*
            const Color primaryColor = offsettedColor;

            const uint8_t scannerPrimaryWidth = 2;

            const uint16_t minIndex = 0;
            const uint16_t maxIndex = neopixelCount - 1;

            int lLimit;
            int rLimit;

            if (direction == LEFT) {
                lLimit = minIndex + scannerPrimaryWidth;
                rLimit = maxIndex - scannerPrimaryWidth;
            } else {
                lLimit = maxIndex - scannerPrimaryWidth;
                rLimit = minIndex + scannerPrimaryWidth;
            }

            const uint16_t currentLimitedIndex = round(map(currentIndex, 0, 255, lLimit, rLimit));

            // Use the half of primary color as secondary color
            const Color secondaryColor = (
                primaryColor.red / 2,
                primaryColor.green / 2,
                primaryColor.blue / 2,
                primaryColor.white / 2
            );

            for (int i = 0; i < neopixelCount; i++) {
                const int l = currentLimitedIndex - scannerPrimaryWidth;
                const int r = currentLimitedIndex + scannerPrimaryWidth;

                if (i == l) {
                    neopixelStrip.setPixelColor(i, secondaryColor); 
                } else if (i > l && i < r) {
                    neopixelStrip.setPixelColor(i, primaryColor); 
                } else if (i == r) {
                    neopixelStrip.setPixelColor(i, secondaryColor); 
                } else {
                    neopixelStrip.setPixelColor(i, 0x000000);
                }
            }

            neopixelStrip.show();
            */
        }

        virtual void updateColor() {
            const Color animationStateColor = calculateAnimationStateColor(animationStepIndex, previousColor, desiredColor);

            // Serial.print("updateColor animationStateColor = ");
            // Serial.print(" r = ");
            // Serial.print(animationStateColor.red);
            // Serial.print(" g = ");
            // Serial.print(animationStateColor.green);
            // Serial.print(" b = ");
            // Serial.print(animationStateColor.blue);
            // Serial.print(" w = ");
            // Serial.print(animationStateColor.white);

            // TODO: Use `fill` instead:
            for (int i = 0; i < neopixelStrip.numPixels(); i++) {
                neopixelStrip.setPixelColor(i, animationStateColor.red, animationStateColor.green, animationStateColor.blue, animationStateColor.white);
            }

            neopixelStrip.show();

            // On last step update previous color
            if (animationStepIndex == ANIMATION_STEP_COUNT - 1) {
                previousColor = animationStateColor;
            }
        }
};

class CathodeStrip : public LEDStrip {

    public:
        Effect supportedEffects[1] = {RAINBOW_CYCLE};

        CathodeStrip(const int8_t pinRed, const int8_t pinGreen, const int8_t pinBlue, const int8_t pinWhite) {
            this->pinRed = pinRed;
            this->pinGreen = pinGreen;
            this->pinBlue = pinBlue;
            this->pinWhite = pinWhite;
        }

        virtual void setup() {
            analogWriteRange(255);

            if (pinRed >= 0) {
                pinMode(pinRed, OUTPUT);
            }

            if (pinGreen >= 0) {
                pinMode(pinGreen, OUTPUT);
            }

            if (pinBlue >= 0) {
                pinMode(pinBlue, OUTPUT);
            }

            if (pinWhite >= 0 && isWhiteSupported) {
                pinMode(pinWhite, OUTPUT);
            }
        }

    private:
        int8_t pinRed = -1;
        int8_t pinGreen = -1;
        int8_t pinBlue = -1;
        int8_t pinWhite = -1;

        virtual void updateColor() {
            /*
            if (pinRed >= 0) {
                analogWrite(pinRed, color.red);
            }

            if (pinGreen >= 0) {
                analogWrite(pinGreen, color.green);
            }

            if (pinBlue >= 0) {
                analogWrite(pinBlue, color.blue);
            }

            if (pinWhite >= 0 && isWhiteSupported) {
                analogWrite(pinWhite, color.white);
            }
            */
        }

        virtual void updateRainbowAnimation() {
            // Not supported
        }

        virtual void updateRainbowCycleAnimation() {
            // TODO: Implement
        }

        virtual void updateLaserscannerAnimation() {
            // Not supported
        }
};

WiFiClientSecure secureWifiClient = WiFiClientSecure();
PubSubClient mqttClient = PubSubClient(secureWifiClient, MQTT_SERVER_TLS_FINGERPRINT);

LEDStrip* ledStrip = nullptr;

const int JSON_BUFFER_SIZE = JSON_OBJECT_SIZE(20);

/*
 * Setup
 */

void setup() {
    Serial.begin(115200);
    delay(250);

    char buffer[64] = {0};
    sprintf(buffer, "setup(): The node '%s' was powered up.", MQTT_CLIENTID);
    Serial.println();
    Serial.println(buffer);

    #ifdef PIN_STATUSLED
        pinMode(PIN_STATUSLED, OUTPUT);
    #endif

    setupWifi();
    setupLEDs();
    setupMQTT();
}

void setupWifi() {
    Serial.printf("setupWifi(): Connecting to to Wi-Fi access point '%s'...\n", WIFI_SSID);

    // Do not store Wi-Fi config in SDK flash area
    WiFi.persistent(false);

    // Disable auto Wi-Fi access point mode
    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        // Blink 2 times when connecting
        blinkStatusLED(2);

        delay(500);
        Serial.println(F("setupWifi(): Connecting..."));
    }

    Serial.print(F("setupWifi(): Connected to Wi-Fi access point. Obtained IP address: "));
    Serial.println(WiFi.localIP());
}

void blinkStatusLED(const int times) {
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

void setupLEDs() {
    Serial.println("setupLEDs(): Setup LEDs...");

    switch (LED_TYPE) {
        case CATHODE:
            ledStrip = new CathodeStrip(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE, PIN_LED_WHITE);
            break;
        case WS2812B:
            ledStrip = new WS2812BStrip(PIN_LED_SIGNAL, LED_COUNT);
            break;
        default:
            ledStrip = nullptr; 
            break;
    }

    const bool isWhiteSupported = LED_CAPABILITY == RGBW;

    ledStrip->setupIsWhiteSupported(isWhiteSupported);
    ledStrip->setupColorCorrectionOffsets(LED_RED_OFFSET, LED_GREEN_OFFSET, LED_BLUE_OFFSET, LED_WHITE_OFFSET);

    // Setup light after setting white supported and color offsets
    ledStrip->setup();

    Color initialColor;
    const uint8_t initialBrightness = 255;
    const int initialTransitionDuration = LED_TRANSITION_DURATION;

    // If native white LEDs are supported, show just them initially
    if (isWhiteSupported) {
        initialColor = {0, 0, 0, 255};
    } else {
        initialColor = {255, 255, 255, 0};
    }

    ledStrip->setState(true);
    ledStrip->setColor(initialColor);
    ledStrip->setBrightness(initialBrightness);
    ledStrip->setTransitionDuration(initialTransitionDuration);
    ledStrip->setEffect(NONE);
    ledStrip->show();
}

void setupMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(onMessageReceivedCallback);
}

void onMessageReceivedCallback(char* topic, byte* payload, unsigned int length) {
    if (!topic || !payload) {
        Serial.println("onMessageReceivedCallback(): Invalid argument (nullpointer) given!");
    } else {
        char payloadMessage[length + 1];

        for (int i = 0; i < length; i++) {
            payloadMessage[i] = (char) payload[i];
        }

        payloadMessage[length] = '\0';

        Serial.printf("onMessageReceivedCallback(): Received message on channel '%s': %s\n", topic, payloadMessage);

        if (updateValuesAccordingJsonMessage(payloadMessage)) {
            publishState();
        } else {
            Serial.println("onMessageReceivedCallback(): The payload could not be parsed as JSON!");
        }
    }
}

/*
    Example payload (RGB+W):
    {
      "state": "ON",
      "color": {
        "b": 0,
        "g": 0,
        "r": 255
      },
      "white_value": 255
      "brightness": 120,
      "transition": 5,
      "effect": "rainbow"
    }

    Example payload (RGB):
    {
      "state": "ON",
      "color": {
        "b": 0,
        "g": 0,
        "r": 255
      },
      "brightness": 120,
      "transition": 5,
      "effect": "rainbow"
    }
*/
bool updateValuesAccordingJsonMessage(char* jsonPayload) {
    bool wasSuccessfulParsed = true;

    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(jsonPayload);

    if (!root.success()) {
        wasSuccessfulParsed = false;
    } else {
        if (root.containsKey("state")) {
            bool state;

            if (strcmp(root["state"], "ON") == 0) {
                state = true;
            } else if (strcmp(root["state"], "OFF") == 0) {
                state = false;
            }

            ledStrip->setState(state);
        }

        // TODO: Check of "r", "b", "g" keys there?
        if (root.containsKey("color")) {
            const Color color = {
                constrainBetweenByte(root["color"]["r"]),
                constrainBetweenByte(root["color"]["g"]),
                constrainBetweenByte(root["color"]["b"]),
                (root.containsKey("white_value")) ? constrainBetweenByte(root["white_value"]) : 0
            };

            ledStrip->setColor(color);
        }

        if (root.containsKey("brightness")) {
            const uint8_t brightness = constrainBetweenByte(root["brightness"]);
            ledStrip->setBrightness(brightness);
        }

        if (root.containsKey("transition")) {
            // The maximum value for `transition` is 60 seconds (to avoid integer overflow)
            const uint32_t transitionDuration = constrain(root["transition"], 0, 60) * 1000000;
            ledStrip->setTransitionDuration(transitionDuration);
        }

        if (root.containsKey("effect")) {
            const Effect effect = effectFromStringRepresentation(root["effect"]);
            ledStrip->setEffect(effect);
        }

        #if DEBUG_LEVEL >= 1
            Serial.print(F("updateValuesAccordingJsonMessage(): The light was changed to:"));
            Serial.print(F(" stateOnOff = "));
            Serial.print(ledStrip->getState());
            Serial.print(F(", brightness = "));
            Serial.print(ledStrip->getBrightness());
            Serial.print(F(", color.red = "));
            Serial.print(ledStrip->getColor().red);
            Serial.print(F(", color.green = "));
            Serial.print(ledStrip->getColor().green);
            Serial.print(F(", color.blue = "));
            Serial.print(ledStrip->getColor().blue);
            Serial.print(F(", color.white = "));
            Serial.print(ledStrip->getColor().white);
            Serial.print(F(", transitionDuration = "));
            Serial.print(ledStrip->getTransitionDuration());
            Serial.print(F(", effect = "));
            Serial.print(effectToStringRepresentation(ledStrip->getEffect()));
            Serial.println();
        #endif

        ledStrip->show();
    }

    return wasSuccessfulParsed;
}

void publishState() {
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    const bool stateOnOff = ledStrip->getState();
    root["state"] = (stateOnOff == true) ? "ON" : "OFF";

    const Color color = ledStrip->getColor();
    JsonObject& jsonColor = root.createNestedObject("color");
    jsonColor["r"] = color.red;
    jsonColor["g"] = color.green;
    jsonColor["b"] = color.blue;

    if (ledStrip->getIsWhiteSupported()) {
        root["white_value"] = color.white;
    }

    const uint8_t brightness = ledStrip->getBrightness();
    root["brightness"] = brightness;

    const Effect effect = ledStrip->getEffect();
    root["effect"] = effectToStringRepresentation(effect);

    char payloadMessage[root.measureLength() + 1];
    root.printTo(payloadMessage, sizeof(payloadMessage));

    Serial.printf("publishState(): Publish message on channel '%s': %s\n", MQTT_CHANNEL_STATE, payloadMessage);
    mqttClient.publish(MQTT_CHANNEL_STATE, payloadMessage, true);
}

void loop() {
    connectMQTT();
    mqttClient.loop();
    ledStrip->loop();
}

void connectMQTT() {
    if (mqttClient.connected() == true) {
        return ;
    }

    Serial.printf("connectMQTT(): Connecting to MQTT broker '%s:%i'...\n", MQTT_SERVER, MQTT_PORT);

    while (mqttClient.connected() == false) {
        Serial.println("connectMQTT(): Connecting...");

        if (mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD) == true) {
            Serial.println("connectMQTT(): Connected to MQTT broker.");

            // (Re)subscribe on topics
            mqttClient.subscribe(MQTT_CHANNEL_COMMAND);

            // Initially publish current state
            publishState();
        } else {
            Serial.printf("connectMQTT(): Connection failed with error code %i. Try again...\n", mqttClient.state());
            blinkStatusLED(3);
            delay(500);
        }
    }
}
