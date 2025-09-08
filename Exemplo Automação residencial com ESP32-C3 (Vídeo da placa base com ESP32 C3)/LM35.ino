#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_ADS1X15.h>

#define dispositivo "sala"

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

// MQTT broker details
const char* mqtt_broker = "215716d34f484f7681398e2cf0125110.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "esp32";
const char* mqtt_password = "Senha1234";

// Pino do sensor LM35 (conectado ao ADS1115)
#define ADS_CHANNEL 0  // Canal A0 do ADS1115

WiFiClientSecure espClient;
PubSubClient client(espClient);

Adafruit_ADS1015 ads; // Usando versão 12-bit

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
  delay(1000);

  Serial.begin(115200);

  // Inicializar ADS1115
  if (!ads.begin(0x48)) { // Endereço padrão 0x48
    Serial.println("Falha ao inicializar ADS1115!");
    while (1);
  }
  Serial.println("ADS1115 inicializado");

  setup_wifi();
  espClient.setInsecure(); // Usar conexão insegura para MQTT sobre TLS
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  pinMode(3, OUTPUT);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Ler valor do ADS1115
  int16_t adc_value = ads.readADC_SingleEnded(ADS_CHANNEL);

  // Converter para tensão (ADS1015 tem fundo de escala de 4.096V em ganho padrão)
  float voltage = ads.computeVolts(adc_value);

  // Converter tensão para temperatura (LM35: 10mV por °C)
  float temperatura = voltage * 100.0;

  Serial.print("Tensão: ");
  Serial.print(voltage, 4);
  Serial.print("V, Temperatura: ");
  Serial.print(temperatura, 2);
  Serial.println("°C");

  char msg[10];
  snprintf(msg, 10, "%.2f", temperatura);

  char topic[50];
  snprintf(topic, sizeof(topic), "%s/%s", dispositivo, "temperatura");
  client.publish(topic, msg);

  for (int x = 0; x < 120; x++) {// Publica a cada 120 segundos
    digitalWrite(3, !digitalRead(3));
    delay(1000); 
  }

}
