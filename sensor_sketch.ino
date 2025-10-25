#include "WiFi.h"
#include "WiFiClientSecure.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include "DHTesp.h"


// WiFi credential
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

WiFiClientSecure esp_client;

// MQTT Broker settings
const char* MQTT_SERVER = "s1f6ee21.ala.asia-southeast1.emqxsl.com"; // DISABLED
const int   MQTT_PORT   = 8883;
const char* MQTT_TOPIC  = "weather/sensors/data";

const char* MQTT_USERNAME = "iot-sensor";
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

// DHT Sensor
const int DHT_PIN = 9; // D2

DHTesp dht_sensor;


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

void connectMqtt() {
  while (!mqtt_client.connected()) {
    Serial.print("Connecting to MQTT broker...");
    String client_id = "esp32-client-" + String(WiFi.macAddress()) + String(random(0xffff), HEX);
    if (mqtt_client.connect(client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("Connected!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void publishSensorData(float temperature, float humidity) {
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = millis();

  char json_buffer[200];
  serializeJson(doc, json_buffer);

  if (mqtt_client.publish(MQTT_TOPIC, json_buffer)) {
    Serial.println("Data published successfully!");
    Serial.println(json_buffer);
  } else {
    Serial.println("Failed to publish data");
  }
}

void setup() {
  Serial.begin(115200);

  connectWifi();

  // Configure secure connection over TLS
  esp_client.setCACert(ca_cert);

  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);

  dht_sensor.setup(DHT_PIN, DHTesp::DHT22);
}

void loop() {
  if (!mqtt_client.connected()) {
    connectMqtt();
  }
  mqtt_client.loop();

  float temperature = dht_sensor.getTemperature();
  float humidity = dht_sensor.getHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    publishSensorData(temperature, humidity);
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  delay(5000); // Publish every 5 seconds
}
