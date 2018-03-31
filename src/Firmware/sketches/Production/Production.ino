#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/*
 * Connection configuration
 */

#define FIRMWARE_VERSION                        "1.0.0"

#define WIFI_SSID                               ""
#define WIFI_PASSWORD                           ""

#define MQTT_CLIENTID                           "Vibelight Device XXXXXXXXXXXXXX"
#define MQTT_SERVER                             ""
#define MQTT_SERVER_TLS_FINGERPRINT             ""
#define MQTT_PORT                               8883
#define MQTT_USERNAME                           "device_XXXXXXXXXXXXXX"
#define MQTT_PASSWORD                           ""

#define MQTT_CHANNEL_COLOR                      "/openhab/vibelight/XXXXXXXXXXXXXX/color/"

// Try to connect N times and reset chip if limit is exceeded
#define CONNECTION_RETRIES                      3

// Uncomment if on the board is a onboard LED
// #define PIN_STATUSLED                           LED_BUILTIN

#define PIN_LED_RED                             14
#define PIN_LED_GREEN                           13
#define PIN_LED_BLUE                            12
#define PIN_LED_WHITE                           11

#define CROSSFADE_ENABLED                       true
#define CROSSFADE_DELAY                         2
#define CROSSFADE_STEPCOUNT                     255

WiFiClientSecure secureWifiClient = WiFiClientSecure();
PubSubClient MQTTClient = PubSubClient(secureWifiClient);

void setup()
{
    Serial.begin(115200);
    delay(250);

    setupPins();
    setupLEDStrip();
    setupWifi();
    setupMQTT();
}

void setupPins()
{
    #ifdef PIN_STATUSLED
        pinMode(PIN_STATUSLED, OUTPUT);
    #endif

    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);   
    pinMode(PIN_LED_BLUE, OUTPUT); 
}

void setupLEDStrip()
{
    Serial.println("Setup LED strip...");

    uint32_t defaultColor = 0xFFFFFF;
    showGivenColor(defaultColor);
}

void showGivenColor(const uint32_t color)
{
    if (CROSSFADE_ENABLED)
    {
        showGivenColorWithFadeEffect(color);
    }
    else
    {
        showGivenColorImmediately(color);
    }
}

void showGivenColorWithFadeEffect(const uint32_t color)
{
    // Split new color into its color parts
    const int newRed = (color >> 16) & 0xFF;
    const int newGreen = (color >> 8) & 0xFF;
    const int newBlue = (color >> 0) & 0xFF;

    // Calulate step count between old and new color value for each color part
    const int stepCountRed = calculateStepsBetweenColorValues(oldRed, newRed);
    const int stepCountGreen = calculateStepsBetweenColorValues(oldGreen, newGreen); 
    const int stepCountBlue = calculateStepsBetweenColorValues(oldBlue, newBlue);

    for (int i = 0; i <= CROSSFADE_STEPCOUNT; i++)
    {
        currentRed = calculateSteppedColorValue(stepCountRed, currentRed, i);
        currentGreen = calculateSteppedColorValue(stepCountGreen, currentGreen, i);
        currentBlue = calculateSteppedColorValue(stepCountBlue, currentBlue, i);

        const int currentColor = (currentRed << 16) | (currentGreen << 8) | (currentBlue << 0);
        showGivenColorImmediately(currentColor);

        delay(CROSSFADE_DELAY);
    }

    oldRed = currentRed; 
    oldGreen = currentGreen; 
    oldBlue = currentBlue;
}

int calculateStepsBetweenColorValues(const int oldColorValue, const int newColorValue)
{
    int colorValueDifference = newColorValue - oldColorValue;

    if (colorValueDifference)
    {
        colorValueDifference = CROSSFADE_STEPCOUNT / colorValueDifference;
    }

    return colorValueDifference;
}

int calculateSteppedColorValue(const int stepCount, const int currentColorValue, const int i) {

    int steppedColorValue = currentColorValue;

    if (stepCount && (i % stepCount) == 0)
    {
        if (stepCount > 0)
        {              
            steppedColorValue += 1;           
        } 
        else if (stepCount < 0)
        {
            steppedColorValue -= 1;
        } 
    }

    steppedColorValue = constrain(steppedColorValue, 0, 255);
    return steppedColorValue;
}

void showGivenColorImmediately(const uint32_t color)
{
    const int rawRed = (color >> 16) & 0xFF;
    const int rawGreen = (color >> 8) & 0xFF;
    const int rawBlue = (color >> 0) & 0xFF;

    int mappedRed = map(rawRed, 0, 255, 0, 1024);
    mappedRed = constrain(mappedRed, 0, 1024);

    int mappedGreen = map(rawGreen, 0, 255, 0, 1024);
    mappedGreen = constrain(mappedGreen, 0, 1024);

    int mappedBlue = map(rawBlue, 0, 255, 0, 1024);
    mappedBlue = constrain(mappedBlue, 0, 1024);

    analogWrite(PIN_LED_RED, mappedRed);
    analogWrite(PIN_LED_GREEN, mappedGreen);
    analogWrite(PIN_LED_BLUE, mappedBlue);
}

void setupWifi()
{
    Serial.printf("Connecting to to Wi-Fi access point '%s'...\n", WIFI_SSID);

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
        Serial.print(F("."));
    }

    Serial.println();

    Serial.print(F("Connected to Wi-Fi access point. Obtained IP address: "));
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
    MQTTClient.setCallback(MQTTRequestCallback);
}

void MQTTRequestCallback(char* topic, byte* payload, unsigned int length)
{
    if (!topic || !payload)
    {
        Serial.println("Invalid argument (nullpointer) given!");
    }
    else
    {
        Serial.printf("Message arrived on channel: %s\n", topic);

        const char* payloadAsCharPointer = (char*) payload;

        if (strcmp(topic, MQTT_CHANNEL_COLOR) == 0)
        {
            const uint32_t desiredColor = getRGBColorFromPayload(payloadAsCharPointer, 0);
            Serial.printf("Set color: %06X\n", desiredColor);

            showGivenColor(desiredColor);
        }
    }
}

uint32_t getRGBColorFromPayload(const char* payload, const uint8_t startPosition)
{
    uint32_t color = 0x000000;

    if (!payload)
    {
        Serial.println("Invalid argument (nullpointer) given!");
    }
    else
    {
        // Pre-initialized char array (length = 7) with terminating null character:
        char rbgColorString[7] = { '0', '0', '0', '0', '0', '0', '\0' };
        strncpy(rbgColorString, payload + startPosition, 6);

        // Convert hexadecimal RGB color strings to decimal integer
        const uint32_t convertedRGBColor = strtol(rbgColorString, NULL, 16);

        // Verify that the given color values are in a valid range
        if ( convertedRGBColor >= 0x000000 && convertedRGBColor <= 0xFFFFFF )
        {
            color = convertedRGBColor;
        }
    }

    return color;
}

void loop()
{
    // High priority for MQTT packets if client is enabled
    if (mqttClientEnabled == true)
    {
        connectMQTT();
        MQTTClient.loop();
    }

    // Lower priority for web interface requests
    webServer.handleClient();
}

void connectMQTT()
{
    if (MQTTClient.connected() == true)
    {
        return ;
    }

    #ifdef CONNECTION_RETRIES
        uint8_t retries = CONNECTION_RETRIES;
    #endif

    while ( MQTTClient.connected() == false )
    {
        Serial.print("Attempting MQTT connection... ");

        if (MQTTClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_SERVER_TLS_FINGERPRINT) == true)
        {
            Serial.println("Connected.");

            // (Re)subscribe on topics
            MQTTClient.subscribe(MQTT_CHANNEL_COLOR);
        }
        else
        {
            Serial.printf("Connection failed! Error code: %i\n", MQTTClient.state());

            // Blink 3 times for indication of failed MQTT connection
            blinkStatusLED(3);

            Serial.println("Try again in 1 second...");
            delay(1000);

            #ifdef CONNECTION_RETRIES
                retries--;

                if (retries == 0)
                {
                    Serial.println("Connection failed too often.");

                    // Exit loop
                    break;
                }
            #endif
        }
    }
}
