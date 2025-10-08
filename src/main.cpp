#include "../lib/secrets.h" // contains Wi-Fi credentials
#include "HomeSpan.h"
#include <Arduino.h>

const int LED_CHANNEL_1_GPIO = 16; // GPIO pin for LED channel 1
const int LED_CHANNEL_2_GPIO = 17; // GPIO pin for LED channel 2
const int LED_CHANNEL_3_GPIO = 18; // GPIO pin for LED channel 3
const int LED_CHANNEL_4_GPIO = 19; // GPIO pin for LED channel 4

struct LightChannel : Service::LightBulb {
  SpanCharacteristic *power;
  SpanCharacteristic *brightness;
  int pin;
  int channel;

  LightChannel(int pin, int channel) : Service::LightBulb() {
    power = new Characteristic::On();
    brightness = new Characteristic::Brightness(100);
    this->pin = pin;
    this->channel = channel;

    ledcSetup(channel, 5000, 8);
    ledcAttachPin(pin, channel);
  }

  boolean update() override {
    if (power->getNewVal()) {
      int duty = brightness->getNewVal() * 255 / 100;
      ledcWrite(channel, duty);
    } else {
      ledcWrite(channel, 0);
    }
    return true;
  }
};

void setup() {
  Serial.begin(115200);

  homeSpan.setPairingCode("111-22-333"); // Must match the ###-##-### format
  homeSpan.setQRID("LED4CH");            // For QR code identification
  homeSpan.setHostNameSuffix("-LED");    // Helps identify device on network

  homeSpan.begin(Category::Lighting, "Andy's Matter LED Controller");

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();

  new LightChannel(LED_CHANNEL_1_GPIO, 0);
  new LightChannel(LED_CHANNEL_2_GPIO, 1);
  new LightChannel(LED_CHANNEL_3_GPIO, 2);
  new LightChannel(LED_CHANNEL_4_GPIO, 3);
}

void loop() { homeSpan.poll(); }
