#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUM_LEDS 144
#define MIDPOINT NUM_LEDS / 2
#define BURST_LENGTH 35
#define BURST_INTERVAL 36
#define EXPLOSION_SIZE 10
#define MIN_SHIFT 3
#define MAX_SHIFT 10
#define WAVE_DELAY 5 // Speed of the wave/chasing effect for spells
#define SHIFT_DELAY 50 // Speed of the shifting middle when both spells are on
#define FAST_SHIFT_DELAY 20 // Speed of the shifting middle when one spell is off
#define FLICKER_DELAY 50 // Delay for flicker effect
#define CRACKLE_FREQUENCY 15 // Frequency of the crackling effect

#define BUTTON_A_PIN 2
#define BUTTON_B_PIN 3

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

bool spellAActive = false;
bool spellBActive = false;

bool lastButtonAState = HIGH;
bool lastButtonBState = HIGH;

// Color arrays for each spell
uint32_t spellAColor = strip.Color(255, 0, 0); // Initial color for Spell A (Red)
uint32_t spellBColor = strip.Color(0, 0, 255); // Initial color for Spell B (Blue)

uint32_t colors[] = {strip.Color(255, 0, 0), strip.Color(0, 0, 255), strip.Color(0, 255, 0), strip.Color(255, 165, 0), strip.Color(128, 0, 128)}; // Red, Blue, Green, Orange, Purple
int numColors = 5;

void setup() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    randomSeed(analogRead(0)); // Seed random number generator

    pinMode(BUTTON_A_PIN, INPUT_PULLUP);
    pinMode(BUTTON_B_PIN, INPUT_PULLUP);

    spellAColor = colors[0]; // Start with Red for Spell A
    spellBColor = colors[1]; // Start with Blue for Spell B
}

void spellFromSideA(uint32_t color, int midpoint, int offset) {
    for (int i = 0; i < midpoint; i++) {
        if ((i - offset) % BURST_INTERVAL < BURST_LENGTH) {
            int brightness = (255 / BURST_LENGTH) * ((i - offset) % BURST_LENGTH);
            uint32_t adjustedColor = color;
            
            // Add crackling effect
            if (random(0, CRACKLE_FREQUENCY) == 0) {
                adjustedColor = lightenColor(color);
            }
            
            uint32_t dimmedColor = dimColor(adjustedColor, brightness);
            strip.setPixelColor(i, dimmedColor);
        }
    }
}

void spellFromSideB(uint32_t color, int midpoint, int offset) {
    int end = NUM_LEDS - 1; // Start from the far end of the strip

    for (int i = end; i >= midpoint; i--) {
        if ((end - i - offset) % BURST_INTERVAL < BURST_LENGTH) {
            int brightness = (255 / BURST_LENGTH) * ((end - i - offset) % BURST_LENGTH);
            uint32_t adjustedColor = color;
            
            // Add crackling effect
            if (random(0, CRACKLE_FREQUENCY) == 0) {
                adjustedColor = lightenColor(color);
            }
            
            uint32_t dimmedColor = dimColor(adjustedColor, brightness);
            strip.setPixelColor(i, dimmedColor);
        }
    }
}

uint32_t blendColors(uint32_t color1, uint32_t color2) {
    int r1 = (color1 >> 16) & 0xFF;
    int g1 = (color1 >> 8) & 0xFF;
    int b1 = color1 & 0xFF;
    
    int r2 = (color2 >> 16) & 0xFF;
    int g2 = (color2 >> 8) & 0xFF;
    int b2 = color2 & 0xFF;
    
    int r = (r1 + r2) / 2; // Average red
    int g = (g1 + g2) / 2; // Average green
    int b = (b1 + b2) / 2; // Average blue
    
    return strip.Color(r, g, b);
}

void createExplosion(int midpoint) {
    static unsigned long lastFlickerTime = 0; // Last time flickering was updated
    
    if (millis() - lastFlickerTime > FLICKER_DELAY) {
        lastFlickerTime = millis(); // Update the last flicker time
        
        // Blend red and blue for saturation
        uint32_t blendedColor = blendColors(spellAColor, spellBColor); // Blend Spell A and Spell B colors
        
        // Set the LEDs to the left of the midpoint
        for (int i = midpoint - EXPLOSION_SIZE; i < midpoint; i++) {
            if (i >= 0) {
                int flicker = random(200, 255); // Increase flicker range for more intensity
                uint32_t color = dimColor(blendedColor, flicker);
                strip.setPixelColor(i, color);
            }
        }
        
        // Set the LEDs to the right of the midpoint
        for (int i = midpoint; i < midpoint + EXPLOSION_SIZE; i++) {
            if (i < strip.numPixels()) {
                int flicker = random(200, 255); // Increase flicker range for more intensity
                uint32_t color = dimColor(blendedColor, flicker);
                strip.setPixelColor(i, color);
            }
        }
    }
}

// Function to dim the color based on a given brightness level
uint32_t dimColor(uint32_t color, uint8_t brightness) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >>  8) & 0xFF;
    uint8_t b =  color        & 0xFF;
    
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    
    return strip.Color(r, g, b);
}

// Function to lighten the color for the crackling effect
uint32_t lightenColor(uint32_t color) {
    uint8_t r = min(255, ((color >> 16) & 0xFF) + 50);
    uint8_t g = min(255, ((color >>  8) & 0xFF) + 50);
    uint8_t b = min(255, (color        & 0xFF) + 50);
    
    return strip.Color(r, g, b);
}

void loop() {
    static int midpoint = MIDPOINT;
    static int targetMidpoint = MIDPOINT;
    static int direction = 1; // Add the direction variable here
    static unsigned long lastShiftTime = 0;
    static int offsetA = 0;
    static int offsetB = 0;
    unsigned long currentShiftDelay = SHIFT_DELAY;

    // Read button states
    bool buttonAState = digitalRead(BUTTON_A_PIN) == LOW;
    bool buttonBState = digitalRead(BUTTON_B_PIN) == LOW;

    // Clear strip before applying new colors
    strip.clear(); 

    if (spellAActive) {
        spellFromSideA(spellAColor, midpoint, offsetA); // Use the current color for Spell A
    }
    if (spellBActive) {
        spellFromSideB(spellBColor, midpoint, offsetB); // Use the current color for Spell B
    }
    
    if (spellAActive || spellBActive) {
        createExplosion(midpoint); // Create the explosion effect at the current midpoint
    }

    strip.show(); // Show the combined result

    // Update offsets for the waves
    offsetA += 1;
    if (offsetA >= BURST_INTERVAL) {
        offsetA = 0;
    }
    
    offsetB += 1;
    if (offsetB >= BURST_INTERVAL) {
        offsetB = 0;
    }

    // Adjust the speed of the midpoint shift based on the spell state
    if ((spellAActive && !spellBActive) || (!spellAActive && spellBActive)) {
        currentShiftDelay = FAST_SHIFT_DELAY;
    } else {
        currentShiftDelay = SHIFT_DELAY;
    }

    // Update shifting midpoint
    if (millis() - lastShiftTime > currentShiftDelay) {
        lastShiftTime = millis(); // Update the last shift time

        if (spellAActive && !spellBActive) {
            targetMidpoint = NUM_LEDS; // Move midpoint to the end of the strip to fully use Spell A
        } else if (!spellAActive && spellBActive) {
            targetMidpoint = 0; // Move midpoint to the start of the strip to fully use Spell B
        } else if (spellAActive && spellBActive) {
            // Reintroduce tug-of-war effect
            targetMidpoint += direction;
            
            // Reverse direction and set a new random shift amount when limits are reached
            if (targetMidpoint >= MIDPOINT + MAX_SHIFT || targetMidpoint <= MIDPOINT - MAX_SHIFT) {
                direction *= -1; // Reverse direction
                targetMidpoint = MIDPOINT; // Reset the midpoint to center for tug-of-war
            }
        }

        // Visibly move midpoint towards the target
        if (midpoint < targetMidpoint) {
            midpoint++;
        } else if (midpoint > targetMidpoint) {
            midpoint--;
        }
    }

    // Toggle spell A
    if (buttonAState && lastButtonAState == HIGH) {
        spellAActive = !spellAActive;
        if (spellAActive) {
            spellAColor = getRandomColor(spellBColor); // Get a new color for Spell A, ensuring it's different from Spell B
        }
        delay(50); // Debounce delay
    }
    lastButtonAState = buttonAState;

    // Toggle spell B
    if (buttonBState && lastButtonBState == HIGH) {
        spellBActive = !spellBActive;
        if (spellBActive) {
            spellBColor = getRandomColor(spellAColor); // Get a new color for Spell B, ensuring it's different from Spell A
        }
        delay(50); // Debounce delay
    }
    lastButtonBState = buttonBState;
}

// Function to get a random color that's different from the specified color
uint32_t getRandomColor(uint32_t differentFrom) {
    uint32_t newColor;
    do {
        newColor = colors[random(0, numColors)];
    } while (newColor == differentFrom);
    return newColor;
}
