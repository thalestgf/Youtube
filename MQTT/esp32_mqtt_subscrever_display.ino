#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Wi-Fi credentials
const char* ssid = "Celular";
const char* password = "123456789";

// MQTT broker details
const char* mqtt_broker = "3336331d013f40ffa18a7e282f88328f.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "ESP32";
const char* mqtt_password = "Senha1234";

// Pinos
#define OLED_SDA 8
#define OLED_SCL 9
#define RELE_PIN 10

// Endereço I2C do display OLED
#define OLED_ADDRESS 0x3C // Use 0x3D se o endereço for diferente

// Inicializa o display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool botao = 0;
String temp;
String umidade;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando à rede: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  int contador = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");

    contador++;
    if (contador > 20) {
      ESP.restart();
    }
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

  if (strcmp(topic, "placa1/temperatura") == 0) {
    temp = payloadStr;
  } else if (strcmp(topic, "placa1/umidade") == 0) {
    umidade = payloadStr;
  } else if (strcmp(topic, "placa1/botao") == 0) {
    int estado = atoi(payloadStr);
    digitalWrite(RELE_PIN, !estado);
  }

  display.display();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("Placa2Client", mqtt_username, mqtt_password)) {
      Serial.println("conectado");
      client.subscribe("placa1/temperatura");
      client.subscribe("placa1/umidade");
      client.subscribe("placa1/botao");
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

  // Inicializa o display OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("Falha ao inicializar o display OLED");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();

  // Configura o pino do relé
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(temp);
  display.print(" C");
  display.setCursor(0, 20);
  display.print("Umidade: ");
  display.print(umidade);
  display.print(" %");
}
