#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define dispositivo "luz"

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

// MQTT broker details
const char* mqtt_broker = "215716d34f484f7681398e2cf0125110.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "esp32";
const char* mqtt_password = "Senha1234";

// Pino do relé
#define RELE_PIN 3

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

  // Converter payload para string
  char payloadStr[length + 1];
  memcpy(payloadStr, payload, length);
  payloadStr[length] = '\0';

  Serial.print("Payload: ");
  Serial.println(payloadStr);

  // Verificar se é o tópico correto
  if (strcmp(topic, "luz/state") == 0) {
    // Controlar o relé baseado no payload
    if (strcmp(payloadStr, "ON") == 0 || strcmp(payloadStr, "1") == 0) {
      digitalWrite(RELE_PIN, HIGH);
      Serial.println("Relé LIGADO");
    } 
    else if (strcmp(payloadStr, "OFF") == 0 || strcmp(payloadStr, "0") == 0) {
      digitalWrite(RELE_PIN, LOW);
      Serial.println("Relé DESLIGADO");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    
    // Gerar um Client ID único
    String clientId = "ESP32-Luz-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("conectado");
      
      // Inscrever no tópico luz/state
      if (client.subscribe("luz/state")) {
        Serial.println("Inscrito no tópico: luz/state");
      } else {
        Serial.println("Falha ao se inscrever no tópico");
      }
      
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);

  // Configurar pino do relé
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW); // Iniciar com relé desligado

  setup_wifi();
  espClient.setInsecure(); // Usar conexão insegura para MQTT sobre TLS
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  Serial.println("Dispositivo Luz inicializado");
  Serial.println("Aguardando comandos no tópico: luz/state");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Manter a conexão ativa
  delay(100);
}
