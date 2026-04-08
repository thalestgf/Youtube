#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 110
#define DATA_PIN 4

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(60);
  strip.clear();
  strip.show();
}

void loop() {
  for (int x = 0; x < 10; x++) {
    strip.setPixelColor(x, strip.Color(0, 255, 0));
    strip.show();
    delay(200);
  }

  strip.clear();
  strip.show();
  delay(200);
}
