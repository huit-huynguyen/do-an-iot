#include "WiFi.h"
#include "WiFiClientSecure.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include "LiquidCrystal_I2C.h"


// WiFi credential
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

WiFiClientSecure esp_client;

// MQTT Broker settings
const char* MQTT_SERVER = "s1f6ee21.ala.asia-southeast1.emqxsl.com"; // DISABLED
const int   MQTT_PORT   = 8883;
const char* MQTT_TOPIC  = "weather/sensors/data";

const char* MQTT_USERNAME = "iot-displayer";
const char* MQTT_PASSWORD = "P@ssw0rd!123";

PubSubClient mqtt_client(esp_client);

static const char ca_cert[]
PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// LCD Display
const int LCD_ADDRESS = 0x27;
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

void connectWifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  float temperature = doc["temperature"];
  float humidity = doc["humidity"];
  unsigned long timestamp = doc["timestamp"];

  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weather Monitor");

  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print(" C");

  lcd.setCursor(0, 2);
  lcd.print("Humidity: ");
  lcd.print(humidity, 1);
  lcd.print(" %");

  lcd.setCursor(0, 3);
  lcd.print("Time: ");
  lcd.print(timestamp / 1000);
  lcd.print("s");

  // Print to Serial
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void connectMqtt() {
  while (!mqtt_client.connected()) {
    Serial.print("Connecting to MQTT broker...");
    String client_id = "esp32-client-" + String(WiFi.macAddress()) + String(random(0xffff), HEX);
    if (mqtt_client.connect(client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("Connected!");
      mqtt_client.subscribe(MQTT_TOPIC);
      Serial.print("Subscribed to topic: ");
      Serial.println(MQTT_TOPIC);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Setup LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  // Connect to WiFi
  connectWifi();

  // Configure secure connection over TLS
  esp_client.setCACert(ca_cert);

  // Setup MQTT
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(mqttCallback);

  connectMqtt();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for data...");
}

void loop() {
  if (!mqtt_client.connected()) {
    connectMqtt();
  }
  mqtt_client.loop();
}
