enum LEDType {
    CATHODE,
    WS2812B
};

enum LEDCapability {
    RGB,
    RGBW
};

typedef struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t white;
};

/**
 * Converts a color into a 32-bit WRGB integer value.
 */
uint32_t colorToInteger(const Color color) {
    return ((uint32_t) color.white << 24) | ((uint32_t) color.red << 16) | ((uint32_t) color.green <<  8) | color.blue;
}

/**
 * Converts a 32-bit WRGB integer to a Color value.
 */
Color integerToColor(const uint32_t color) {
    return Color {
        (color >> 24 & 0xFF),
        (color >> 16 & 0xFF),
        (color >>  8 & 0xFF),
        (color >>  0 & 0xFF)
    };
}

enum Effect {
    NONE,
    RAINBOW,
    RAINBOW_CYCLE,
    LASERSCANNER
};

#define EFFECT_STRING_NONE "none"
#define EFFECT_STRING_RAINBOW "rainbow"
#define EFFECT_STRING_RAINBOW_CYCLE "rainbowCycle" // TODO: Rename to colorloop
#define EFFECT_STRING_LASERSCANNER "laserScanner"

char* effectToStringRepresentation(enum Effect effect) {
    char* stringRepresentations[] = {
        EFFECT_STRING_NONE,
        EFFECT_STRING_RAINBOW,
        EFFECT_STRING_RAINBOW_CYCLE,
        EFFECT_STRING_LASERSCANNER
    };

    return stringRepresentations[effect];
}

Effect effectFromStringRepresentation(const char* effectStringRepresentation) {
    Effect effect;

    if (strcmp(effectStringRepresentation, EFFECT_STRING_RAINBOW) == 0) {
        effect = RAINBOW;
    } else if (strcmp(effectStringRepresentation, EFFECT_STRING_RAINBOW_CYCLE) == 0) {
        effect = RAINBOW_CYCLE;
    } else if (strcmp(effectStringRepresentation, EFFECT_STRING_LASERSCANNER) == 0) {
        effect = LASERSCANNER;
    } else {
        effect = NONE;
    }

    return effect;
}

enum EffectDirection {
    LEFT,
    RIGHT
};
