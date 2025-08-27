#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// Wi-Fi credentials
const char* ssid = "Celular";
const char* password = "123456789";

// MQTT broker details
const char* mqtt_broker = "3336331d013f40ffa18a7e282f88328f.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "ESP32";
const char* mqtt_password = "Senha1234";

// Pinos
#define DHTPIN 1          // Pino onde o DHT11 está conectado
#define BOTAO_PIN 9       // Pino do botão

// Inicializa o DHT
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  char payloadStr[length + 1];
  memcpy(payloadStr, payload, length);
  payloadStr[length] = '\0';

  Serial.print("Payload: ");
  Serial.println(payloadStr);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("Placa1Client", mqtt_username, mqtt_password)) {
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
  client.setCallback(callback);
  dht.begin();
  pinMode(BOTAO_PIN, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Ler temperatura e umidade
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  // Publicar temperatura e umidade
  if (!isnan(temperatura)) {
    char tempStr[8];
    snprintf(tempStr, 8, "%.2f", temperatura);
    client.publish("placa1/temperatura", tempStr);
  }
  if (!isnan(umidade)) {
    char umidStr[8];
    snprintf(umidStr, 8, "%.2f", umidade);
    client.publish("placa1/umidade", umidStr);
  }

  // Ler e publicar estado do botão
  int botaoEstado = digitalRead(BOTAO_PIN);
  char msg[2];
  snprintf(msg, 2, "%d", botaoEstado);
  client.publish("placa1/botao", msg);

  delay(2000); // Publica a cada 2 segundos
}
