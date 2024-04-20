#include "Arduino.h"
#include "ESP32MQTTClient.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#define DHTPIN 4
#include <SPI.h>

DHTesp dht;

const char *ssid = "INFINITUMFAAD_2.4";  // Enter your WiFi name
const char *password = "MxgmxNPd2P";

const char *server = "mqtt://52.206.90.192";
const char *subscribeTopic = "mqtt/metrics/esp";
const char *publishTopic = "mqtt/metrics/esp";

const int placaTermica = 12;
const int motorAgua = 32;
const int agua = 34;
const int uv = 5;

const int codeEsp = 616;

ESP32MQTTClient mqttClient;

int id;
int max_temp;
int min_temp;
int max_humidity;
int min_humidity;
int max_uv;
int min_uv;

void setup() {
  Serial.begin(115200);

  pinMode(motorAgua, OUTPUT);
  pinMode(agua, INPUT);
  pinMode(uv, INPUT);
  
  dht.setup(DHTPIN, DHTesp::DHT11);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conexión exitosa a WiFi");

  mqttClient.enableDebuggingMessages();
  mqttClient.setURI(server);
  mqttClient.enableLastWillMessage("lwt", "fuera de linea");
  mqttClient.setKeepAlive(30);
  WiFi.setHostname("c3test");
  mqttClient.loopStart();
}



void loop() {
  digitalWrite(motorAgua, LOW);
  
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  int valorAgua = analogRead(agua);
  int valorUV = analogRead(uv);
  
  Serial.println(humidity);
  Serial.println(temperature);
  Serial.println(valorAgua);
  Serial.println(valorUV);

  // Crear un objeto JSON
  JsonDocument jsonDocument;
  jsonDocument["humedad"] = humidity;
  jsonDocument["temperatura"] = temperature;
  jsonDocument["agua"] = valorAgua;
  jsonDocument["uv"] = valorUV;
  jsonDocument["codeEsp"] = codeEsp;
  jsonDocument["id"] = id;
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  Serial.println("Enviando datos");
  mqttClient.publish(publishTopic, jsonString.c_str(), 0, false);

  if (humidity < min_humidity){
    digitalWrite(motorAgua, HIGH);
    Serial.println("Agua encendido");
  }
  
  delay(5000);
}

void onConnectionEstablishedCallback(esp_mqtt_client_handle_t client) {
  if (mqttClient.isMyTurn(client)) {
    mqttClient.subscribe(subscribeTopic, [](const String &payload) {
      Serial.printf("%s: %s\n", subscribeTopic, payload.c_str());
      
    });
    
    mqttClient.subscribe("mqtt/metrics", [](const String &topic, const String &payload) {
      JsonDocument jsonDocument;
      DeserializationError error = deserializeJson(jsonDocument, payload);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      id = jsonDocument["id"];
      max_temp = jsonDocument["max_temp"];
      min_temp = jsonDocument["min_temp"];
      max_humidity = jsonDocument["max_humidity"];
      min_humidity = jsonDocument["min_humidity"];
      max_uv = jsonDocument["max_uv"];
      min_uv = jsonDocument["min_uv"];
      Serial.printf("%s: %s\n", topic.c_str(), payload.c_str());
    });
  }
}

esp_err_t handleMQTT(esp_mqtt_event_handle_t event) {
  mqttClient.onEventCallback(event);
  return ESP_OK;
}
