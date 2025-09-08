#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configurações do OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

// MQTT broker details
const char* mqtt_broker = "215716d34f484f7681398e2cf0125110.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "esp32";
const char* mqtt_password = "Senha1234";

// Variáveis para armazenar dados dos dispositivos
float salaTemperatura = 0.0;
float quartoTemperatura = 0.0;
String luzState = "OFF";
float estufaTemperatura = 0.0;
float estufaCorrente = 0.0;
String estufaReleState = "OFF"; // Novo: estado do relé da estufa

// Controle de exibição no OLED
int dispositivoAtual = 0; // 0=Sala, 1=Quarto, 2=Luz, 3=Estufa
unsigned long ultimaTrocaDisplay = 0;
const unsigned long INTERVALO_DISPLAY = 10000; // 10 segundos

// Web Server
WebServer server(80);
unsigned long ultimaAtualizacaoWeb = 0;
const unsigned long INTERVALO_WEB = 3000; // 5 segundos

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

  // Processar mensagens baseado no tópico
  if (strcmp(topic, "sala/temperatura") == 0) {
    salaTemperatura = atof(payloadStr);
  } 
  else if (strcmp(topic, "quarto/temperatura") == 0) {
    quartoTemperatura = atof(payloadStr);
  } 
  else if (strcmp(topic, "luz/state") == 0) {
    luzState = String(payloadStr);
  } 
  else if (strcmp(topic, "estufa/temperatura") == 0) {
    estufaTemperatura = atof(payloadStr);
  } 
  else if (strcmp(topic, "estufa/corrente") == 0) {
    estufaCorrente = atof(payloadStr);
  }
  else if (strcmp(topic, "estufa/rele/state") == 0) {
    estufaReleState = String(payloadStr);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    
    String clientId = "ESP32-Monitor-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("conectado");
      
      // Inscrever nos tópicos necessários
      client.subscribe("sala/temperatura");
      client.subscribe("quarto/temperatura");
      client.subscribe("luz/state");
      client.subscribe("estufa/temperatura");
      client.subscribe("estufa/corrente");
      client.subscribe("estufa/rele/state");
      
      Serial.println("Inscrito em todos os tópicos");
      
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void atualizarDisplay() {
  display.clearDisplay();
  
  // Linha amarela (cabeçalho)
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  
  switch (dispositivoAtual) {
    case 0: // Sala
      display.println("SALA");
      break;
      
    case 1: // Quarto
      display.println("QUARTO");
      break;
      
    case 2: // Luz
      display.println("LUZ");
      break;
      
    case 3: // Estufa
      display.println("ESTUFA");
      break;
  }
  
  // Linha divisória

  
  // Conteúdo principal (linhas azuis)
  display.setTextSize(2);
  display.setCursor(0, 16);
  
  switch (dispositivoAtual) {
    case 0: // Sala
      display.print(salaTemperatura, 1);
      display.println(" C");
      break;
      
    case 1: // Quarto
      display.print(quartoTemperatura, 1);
      display.println(" C");
      break;
      
    case 2: // Luz
      display.println(luzState);
      break;
      
    case 3: // Estufa
      display.setTextSize(2);
      display.print("T: ");
      display.print(estufaTemperatura, 1);
      display.println(" C");
      display.print("I: ");
      display.print(estufaCorrente, 2);
      display.println(" A");
      display.print("Rl: ");
      display.print(estufaReleState);
      break;
  }
  
  display.display();
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='5'>"; // Atualiza a cada 5 segundos
  html += "<title>Monitor MQTT</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += ".card { border: 1px solid #ddd; padding: 15px; margin: 10px 0; border-radius: 5px; }";
  html += ".on { background-color: #d4edda; }";
  html += ".off { background-color: #f8d7da; }";
  html += "button { padding: 10px 15px; margin: 5px; border: none; border-radius: 4px; cursor: pointer; }";
  html += ".btn-on { background-color: #28a745; color: white; }";
  html += ".btn-off { background-color: #dc3545; color: white; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Monitor de Dispositivos MQTT</h1>";
  
  // Sala
  html += "<div class='card'>";
  html += "<h2>Sala</h2>";
  html += "<p>Temperatura: " + String(salaTemperatura, 1) + " &deg;C</p>"; // Corrigido: &deg; em vez de °
  html += "</div>";
  
  // Quarto
  html += "<div class='card'>";
  html += "<h2>Quarto</h2>";
  html += "<p>Temperatura: " + String(quartoTemperatura, 1) + " &deg;C</p>"; // Corrigido: &deg; em vez de °
  html += "</div>";
  
  // Luz
  html += "<div class='card ";
  html += (luzState == "ON" ? "on" : "off");
  html += "'>";
  html += "<h2>Luz</h2>";
  html += "<p>Estado: " + luzState + "</p>";
  html += "<form action='/luz' method='POST'>";
  html += "<button class='btn-on' type='submit' name='state' value='ON'>Ligar</button>";
  html += "<button class='btn-off' type='submit' name='state' value='OFF'>Desligar</button>";
  html += "</form>";
  html += "</div>";
  
  // Estufa
  html += "<div class='card ";
  html += (estufaReleState == "ON" ? "on" : "off");
  html += "'>";
  html += "<h2>Estufa</h2>";
  html += "<p>Temperatura: " + String(estufaTemperatura, 1) + " &deg;C</p>"; // Corrigido: &deg; em vez de °
  html += "<p>Corrente: " + String(estufaCorrente, 2) + " A</p>";
  html += "<p>Rele: " + estufaReleState + "</p>"; // Novo: estado do relé
  html += "<form action='/estufa' method='POST'>";
  html += "<button class='btn-on' type='submit' name='state' value='ON'>Ligar Rele</button>";
  html += "<button class='btn-off' type='submit' name='state' value='OFF'>Desligar Rele</button>";
  html += "</form>";
  html += "</div>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleLuz() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    client.publish("luz/state", state.c_str());
    luzState = state;
    Serial.println("Comando enviado para luz: " + state);
  }
  handleRoot();
}

void handleEstufa() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    client.publish("estufa/rele/state", state.c_str());
    Serial.println("Comando enviado para estufa: " + state);
  }
  handleRoot();
}

void setup() {
  delay(1000);
  Serial.begin(115200);

  // Inicializar display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do SSD1306"));
    for(;;); // Trava o programa
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  
  // Configurar cores (se seu display suportar)
  // Nota: A maioria dos displays OLED monocromáticos não suporta cores,
  // então simulamos com diferentes intensidades/brilho
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Iniciando...");
  display.display();

  setup_wifi();
  
  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/luz", handleLuz);
  server.on("/estufa", handleEstufa);
  server.begin();
  Serial.println("Servidor HTTP iniciado");

  espClient.setInsecure(); // Usar conexão insegura para MQTT sobre TLS
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  Serial.println("Sistema de monitoramento inicializado");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient();

  unsigned long agora = millis();

  // Alternar display a cada 10 segundos
  if (agora - ultimaTrocaDisplay >= INTERVALO_DISPLAY) {
    ultimaTrocaDisplay = agora;
    dispositivoAtual = (dispositivoAtual + 1) % 4; // Cicla entre 0, 1, 2, 3
    atualizarDisplay();
  }

  // Pequeno delay para evitar sobrecarga
  delay(100);
}
