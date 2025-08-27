#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// Wi-Fi credentials
const char* ssid = "Celular";
const char* password = "123456789";

// MQTT broker details
const char* mqtt_broker = "3336331d013f40ffa18a7e282f88328f.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "ESP32";
const char* mqtt_password = "Senha1234";

// Pino do botão
const int botaoPin = 9;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando à rede: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("conectado");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  espClient.setInsecure(); // Usar conexão insegura para MQTT sobre TLS
  client.setServer(mqtt_broker, mqtt_port);
  pinMode(botaoPin, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int botaoEstado = digitalRead(botaoPin);
  char msg[2];
  snprintf(msg, 2, "%d", botaoEstado);
  client.publish("botao", msg);
  delay(1000); // Publica a cada 1 segundo
}
