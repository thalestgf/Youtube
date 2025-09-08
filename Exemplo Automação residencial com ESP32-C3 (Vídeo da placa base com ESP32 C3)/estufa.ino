#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <EmonLib.h>
#include <DHT.h>

#define dispositivo "estufa"
#define topico_rele "estufa/rele/state"

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

// MQTT broker details
const char* mqtt_broker = "215716d34f484f7681398e2cf0125110.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "esp32";
const char* mqtt_password = "Senha1234";

// Configuração dos sensores e relé
#define RELE_PIN 3           // Pino do relé
#define CORRENTE_PIN 0       // Pino do sensor de corrente (SCT-013)
#define DHT_PIN 2            // Pino do sensor DHT11 (altere conforme sua conexão)
#define DHT_TYPE DHT11       // Tipo do sensor DHT

// Fator de calibração para o SCT-013 (ajuste conforme necessário)
#define CALIBRACAO_CORRENTE 18.3

// Intervalos de publicação
unsigned long ultimaLeituraCorrente = 0;
unsigned long ultimaLeituraTemperatura = 0;
const unsigned long INTERVALO_CORRENTE = 5000;    // 5 segundos
const unsigned long INTERVALO_TEMPERATURA = 30000; // 30 segundos

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Objetos dos sensores
EnergyMonitor emon;
DHT dht(DHT_PIN, DHT_TYPE);

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

  // Verificar se é o tópico correto para controlar o relé
  if (strcmp(topic, topico_rele) == 0) {
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
    String clientId = "ESP32-Estufa-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("conectado");

      // Inscrever no tópico
      char topic[50];
      snprintf(topic, sizeof(topic), "%s/%s", dispositivo, "rele/state");
      if (client.subscribe(topic)) {
        Serial.println("Inscrito no tópico: rele/state");
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

void publicarCorrente() {
  // Ler valor de corrente
  double corrente = emon.calcIrms(10000); // 1480 número de amostras

  Serial.print("Corrente: ");
  Serial.print(corrente, 4);
  Serial.println("A");

  // Publicar no MQTT
  char msg[10];
  snprintf(msg, 10, "%.4f", corrente);

  char topic[50];
  snprintf(topic, sizeof(topic), "%s/%s", dispositivo, "corrente");
  client.publish(topic, msg);
}

void publicarTemperatura() {
  // Ler temperatura do DHT11
  float temperatura = dht.readTemperature();

  // Verificar se a leitura foi bem-sucedida
  if (isnan(temperatura)) {
    Serial.println("Falha na leitura do DHT11!");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura, 2);
  Serial.println("°C");

  // Publicar no MQTT
  char msg[10];
  snprintf(msg, 10, "%.2f", temperatura);

  char topic[50];
  snprintf(topic, sizeof(topic), "%s/%s", dispositivo, "temperatura");
  client.publish(topic, msg);
}

void setup() {
  delay(1000);
  Serial.begin(115200);

  // Configurar pino do relé
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW); // Iniciar com relé desligado

  // Inicializar sensor de corrente
  emon.current(CORRENTE_PIN, CALIBRACAO_CORRENTE);

  // Inicializar sensor DHT11
  dht.begin();

  setup_wifi();
  espClient.setInsecure(); // Usar conexão insegura para MQTT sobre TLS
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  Serial.println("Dispositivo Estufa inicializado");
  Serial.println("Monitorando corrente e temperatura");
  Serial.println("Aguardando comandos no tópico: rele/state");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long agora = millis();

  // Publicar corrente a cada 5 segundos
  if (agora - ultimaLeituraCorrente >= INTERVALO_CORRENTE) {
    ultimaLeituraCorrente = agora;
    publicarCorrente();
  }

  // Publicar temperatura a cada 30 segundos
  if (agora - ultimaLeituraTemperatura >= INTERVALO_TEMPERATURA) {
    ultimaLeituraTemperatura = agora;
    publicarTemperatura();
  }

  // Pequeno delay para evitar sobrecarga
  delay(100);
}
