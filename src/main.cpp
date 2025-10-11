#include "../lib/secrets.h" // contains Wi-Fi credentials
#include "HomeSpan.h"
#include <Arduino.h>
#include <WiFi.h>

const int LED_CHANNEL_1_GPIO = 25; // GPIO pin for LED channel 1
const int LED_CHANNEL_2_GPIO = 27; // GPIO pin for LED channel 2
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

    Serial.print("Initializing LED Channel ");
    Serial.print(channel);
    Serial.print(" on GPIO ");
    Serial.println(pin);

    ledcSetup(channel, 5000, 8);
    ledcAttachPin(pin, channel);
  }

  boolean update() override {
    // Debug output what's been set from HomeKit
    Serial.print("LED Channel ");
    Serial.print(channel);
    Serial.print(" (GPIO ");
    Serial.print(pin);
    Serial.print(") - Power: ");
    Serial.print(power->getNewVal() ? "ON" : "OFF");
    Serial.print(", Brightness: ");
    Serial.print(brightness->getNewVal());

    if (power->getNewVal()) {
      int duty = brightness->getNewVal() * 255 / 100;
      Serial.print(" -> PWM Duty: ");
      Serial.println(duty);
      ledcWrite(channel, duty);
    } else {
      Serial.println(" -> PWM Duty: 0");
      ledcWrite(channel, 0);
    }
    return true;
  }
};

void wifiScan() {
  // Scan for available networks to verify SSID exists
  Serial.println("Scanning for available Wi-Fi networks...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found!");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
    bool ssidFound = false;
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm) ");
      Serial.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted");
      if (WiFi.SSID(i) == WIFI_SSID) {
        Serial.print(" <- TARGET NETWORK FOUND!");
        ssidFound = true;
      }
      Serial.println();
    }
    if (!ssidFound) {
      Serial.println("WARNING: Target SSID not found in scan!");
    }
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  Serial.println("=== Matter LED Controller Starting ===");

  // Initialize Wi-Fi hardware first - for some reason HomeSpan is slow
  Serial.println("Initializing Wi-Fi hardware...");
  WiFi.mode(WIFI_STA);
  delay(100);

  // Check if Wi-Fi hardware is working
  Serial.print("Wi-Fi MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (WiFi.macAddress() == "00:00:00:00:00:00") {
    Serial.println("ERROR: Wi-Fi hardware not detected!");
    Serial.println("This could be a hardware issue or power problem.");
    while (1)
      delay(1000); // Stop here if no Wi-Fi hardware
  }

  Serial.print("Connecting to Wi-Fi network: ");
  Serial.println(WIFI_SSID);
  Serial.print("Password length: ");
  Serial.println(strlen(WIFI_PASSWORD));

  // wifiScan();

  // Try manual Wi-Fi connection first to debug
  Serial.println("Attempting manual Wi-Fi connection...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for initial connection attempt
  for (int i = 0; i < 10; i++) {
    delay(1000);
    Serial.print("Manual attempt ");
    Serial.print(i + 1);
    Serial.print("/10 - Status: ");
    Serial.println(WiFi.status());
    if (WiFi.status() == WL_CONNECTED)
      break;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Manual Wi-Fi connection successful!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("Keeping connection active for HomeSpan...");
  } else {
    Serial.println("Manual Wi-Fi connection failed, letting HomeSpan handle it...");
  }

  // Set Wi-Fi credentials for HomeSpan (it will use existing connection if available)
  homeSpan.setWifiCredentials(WIFI_SSID, WIFI_PASSWORD);

  homeSpan.setPairingCode("11122333"); // Must be exactly 8 digits
  homeSpan.setQRID("LED4CH");          // For QR code identification
  homeSpan.setHostNameSuffix("-LED");  // Helps identify device on network

  homeSpan.begin(Category::Lighting, "Andy's Matter LED Controller");

  // Wait for Wi-Fi connection and display status
  Serial.println("Waiting for Wi-Fi connection...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    attempts++;
    Serial.print(".");
    if (attempts % 10 == 0) {
      Serial.println();
      Serial.print("Wi-Fi status: ");
      switch (WiFi.status()) {
      case WL_IDLE_STATUS:
        Serial.println("IDLE");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("NO SSID AVAILABLE");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("SCAN COMPLETED");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("CONNECT FAILED");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("CONNECTION LOST");
        break;
      case WL_DISCONNECTED:
        Serial.println("DISCONNECTED");
        break;
      default:
        Serial.print("UNKNOWN (");
        Serial.print(WiFi.status());
        Serial.println(")");
        break;
      }
      Serial.print("Attempt ");
      Serial.print(attempts);
      Serial.println("/30");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("=== Wi-Fi Connected Successfully! ===");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println();
    Serial.println("=== Wi-Fi Connection Failed! ===");
    Serial.println("Check your SSID and password in secrets.h");
    Serial.println("Device will continue but may not function properly");
  }

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();

  Serial.println("Creating LED channels...");

  // Test GPIO pins directly before creating HomeSpan channels
  Serial.println("=== Quick LED Test ===");

  const int pins[] = {LED_CHANNEL_1_GPIO, LED_CHANNEL_2_GPIO, LED_CHANNEL_3_GPIO,
                      LED_CHANNEL_4_GPIO};
  const int numPins = sizeof(pins) / sizeof(pins[0]);

  for (int i = 0; i < numPins; i++) {
    pinMode(pins[i], OUTPUT);
    Serial.print("Testing GPIO ");
    Serial.print(pins[i]);
    Serial.println("...");
    digitalWrite(pins[i], HIGH);
    delay(250);
    digitalWrite(pins[i], LOW);
    delay(100);
  }

  Serial.println("=== LED Test Complete ===");

  // Create all channels now that we know most are working
  new LightChannel(LED_CHANNEL_1_GPIO, 0);
  Serial.println("Channel 1 created");
  new LightChannel(LED_CHANNEL_2_GPIO, 1);
  Serial.println("Channel 2 created");
  new LightChannel(LED_CHANNEL_3_GPIO, 2);
  Serial.println("Channel 3 created");
  new LightChannel(LED_CHANNEL_4_GPIO, 3);
  Serial.println("Channel 4 created");

  Serial.println("=== Setup Complete ===");
}

void loop() {
  homeSpan.poll();

  // Check Wi-Fi status every 30 seconds
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 30000) {
    lastWiFiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WARNING: Wi-Fi connection lost!");
      Serial.print("Current status: ");
      switch (WiFi.status()) {
      case WL_IDLE_STATUS:
        Serial.println("IDLE");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("NO SSID AVAILABLE");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("CONNECT FAILED");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("CONNECTION LOST");
        break;
      case WL_DISCONNECTED:
        Serial.println("DISCONNECTED");
        break;
      default:
        Serial.print("UNKNOWN (");
        Serial.print(WiFi.status());
        Serial.println(")");
        break;
      }
    } else {
      // Connection is good, show signal strength
      Serial.print("Wi-Fi OK - IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(", RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    }
  }
}
