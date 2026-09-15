/*
  WarmLink - Endpunkt 1
  Liest Temperatur und Feuchtigkeit, sendet und empfaengt Werte per
  MQTT, zeigt Temperatur der Gegenseite per LED an, Button sendet
  einen Verbundenheits-Puls.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

// ==================== KONFIGURATION ====================
#define ENDPOINT_ID 1

// WLAN-Zugangsdaten
const char* WIFI_SSID     = "DEIN_WLAN_NAME";
const char* WIFI_PASSWORD = "DEIN_WLAN_PASSWORT";

// MQTT-Broker
const char* MQTT_BROKER   = "192.168.0.100";
const int   MQTT_PORT     = 1883;

// Pins
#define DHT_PIN       D3     // Temperatursensor
#define DHT_TYPE      DHT11
#define LED_PIN       D1     // RGB-LED
#define LED_COUNT     4
#define BUTTON_PIN    D2     // Button

// =========================================================

String OWN_TOPIC     = "warmlink/endpoint" + String(ENDPOINT_ID) + "/temp";
String OTHER_TOPIC   = "warmlink/endpoint" + String(ENDPOINT_ID == 1 ? 2 : 1) + "/temp";
String OWN_HUMIDITY  = "warmlink/endpoint" + String(ENDPOINT_ID) + "/humidity";
String OWN_PULSE     = "warmlink/endpoint" + String(ENDPOINT_ID) + "/pulse";
String OTHER_PULSE   = "warmlink/endpoint" + String(ENDPOINT_ID == 1 ? 2 : 1) + "/pulse";

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

float receivedTemp = 20.0;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 8000;

bool pulseReceived = false;
bool lastButtonState = false;

// WLAN verbinden
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Serial.print("Verbinde mit WLAN: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWLAN verbunden! IP-Adresse: " + WiFi.localIP().toString());
}

// MQTT Nachricht empfangen
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  String topicStr = String(topic);
  Serial.println("Nachricht empfangen [" + topicStr + "]: " + message);

  if (topicStr == OTHER_TOPIC) {
    receivedTemp = message.toFloat();
  } else if (topicStr == OTHER_PULSE) {
    pulseReceived = true;
  }
}

// MQTT verbinden
void connectMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Verbinde mit MQTT-Broker...");
    String clientId = "WarmLinkEndpoint" + String(ENDPOINT_ID);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" verbunden!");
      mqttClient.subscribe(OTHER_TOPIC.c_str());
      mqttClient.subscribe(OTHER_PULSE.c_str());
      Serial.println("Abonniert: " + OTHER_TOPIC + " und " + OTHER_PULSE);
    } else {
      Serial.print(" fehlgeschlagen, Fehlercode=");
      Serial.print(mqttClient.state());
      Serial.println(" -> neuer Versuch in 3 Sekunden");
      delay(3000);
    }
  }
}

// Temperatur in Farbe umrechnen
struct ColorStop { float temp; int r, g, b; };
ColorStop colorStops[] = {
  {0.0,    0,   0, 255},
  {10.0,   0, 255,   0},
  {20.0, 255, 255,   0},
  {30.0, 255, 140,   0},
  {40.0, 255,   0,   0}
};
const int NUM_STOPS = 5;

uint32_t temperatureToColor(float temp) {
  temp = constrain(temp, colorStops[0].temp, colorStops[NUM_STOPS - 1].temp);
  for (int i = 0; i < NUM_STOPS - 1; i++) {
    if (temp >= colorStops[i].temp && temp <= colorStops[i + 1].temp) {
      float range = colorStops[i + 1].temp - colorStops[i].temp;
      float ratio = (temp - colorStops[i].temp) / range;
      int r = colorStops[i].r + ratio * (colorStops[i + 1].r - colorStops[i].r);
      int g = colorStops[i].g + ratio * (colorStops[i + 1].g - colorStops[i].g);
      int b = colorStops[i].b + ratio * (colorStops[i + 1].b - colorStops[i].b);
      return strip.Color(r, g, b);
    }
  }
  return strip.Color(255, 255, 255);
}

void updateLed(float temp) {
  uint32_t color = temperatureToColor(temp);
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
  strip.show();
}

// Puls-Animation abspielen
void playPulseAnimation() {
  Serial.println("Puls empfangen! Zeige Blink-Animation...");
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < LED_COUNT; j++) strip.setPixelColor(j, strip.Color(255, 255, 255));
    strip.show();
    delay(200);
    for (int j = 0; j < LED_COUNT; j++) strip.setPixelColor(j, strip.Color(0, 0, 0));
    strip.show();
    delay(200);
  }
  updateLed(receivedTemp);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nWarmLink Endpunkt " + String(ENDPOINT_ID) + " startet...");

  dht.begin();
  strip.begin();
  strip.setBrightness(80);
  strip.show();

  pinMode(BUTTON_PIN, INPUT);

  connectWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setSocketTimeout(10);
}

void loop() {
  if (!mqttClient.connected()) {
    connectMqtt();
  }
  mqttClient.loop();

  // Eigenen Sensorwert senden
  if (millis() - lastSendTime > SEND_INTERVAL) {
    lastSendTime = millis();

    float ownTemp = dht.readTemperature();
    float ownHumidity = dht.readHumidity();

    if (isnan(ownTemp)) {
      Serial.println("Fehler beim Lesen des Sensors!");
    } else {
      Serial.println("Eigene Temperatur: " + String(ownTemp) + " C");
      String tempString = String(ownTemp, 1);
      mqttClient.publish(OWN_TOPIC.c_str(), tempString.c_str());

      if (!isnan(ownHumidity)) {
        String humString = String(ownHumidity, 1);
        mqttClient.publish(OWN_HUMIDITY.c_str(), humString.c_str());
      }
    }

    updateLed(receivedTemp);
  }

  // Button pruefen
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState && !lastButtonState) {
    Serial.println("Button gedrueckt! Sende Puls...");
    mqttClient.publish(OWN_PULSE.c_str(), "1");
  }
  lastButtonState = currentButtonState;

  // Empfangenen Puls abspielen
  if (pulseReceived) {
    pulseReceived = false;
    playPulseAnimation();
  }
}
