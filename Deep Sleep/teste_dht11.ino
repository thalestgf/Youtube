#include <Arduino.h>
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>  // Biblioteca para fazer requisições HTTP
#include <ArduinoJson.h> // Biblioteca para manipulação de JSON
#include "esp_sleep.h"

// Configurações
#define DHT_PIN 1          // Pino 1 para o DHT11
#define DHT_TYPE DHT11     // Tipo do sensor
#define VOLTAGE_PIN 3      // Pino analógico para medir a tensão da bateria
#define SLEEP_MINUTES 10   // Tempo de deep sleep em minutos (ajuste conforme necessário)
#define LED 8

// Credenciais WiFi (atualizadas com os dados fornecidos)
const char* ssid = "Desktop_F7717477";
const char* password = "Tt08051993";

char googleScriptURL[200] = "https://script.google.com/macros/s/AKfycbxkEycbmrj6yImkkJfwwO0zhGwTEYa4AW9mGURlgvaON90UPcEfiSfmMU81CHMigBxzrg/exec";

DHT dht(DHT_PIN, DHT_TYPE);

// Protótipo da função de envio para planilha
void sendToGoogleSheet(float temp, float humidity, float voltage);

void setup() {
  Serial.begin(115200);
  //while(!Serial); // Apenas para depuração, remova em produção
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();
  // Inicializa sensor DHT
  dht.begin();

  // Configura pino de tensão da bateria
  pinMode(VOLTAGE_PIN, INPUT);

  pinMode(LED, OUTPUT);

  // Medições
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();

  int rawValue;
  float voltage;

  do {
    // Medição da tensão da bateria
    rawValue = analogRead(VOLTAGE_PIN);
    voltage = rawValue * (3.3 / 4095.0) * (1.93 / 2.2) * 2.0; // Divisor de tensão 10k+10k


    // Verifica se as leituras do DHT são válidas
    if (isnan(humidity) || isnan(temp)) {
      Serial.println("Falha na leitura do DHT!");
      humidity = -1;
      temp = -1;
    }

    if (Serial) {
      Serial.print("Temperatura: "); Serial.print(temp); Serial.println(" °C");
      Serial.print("Umidade: "); Serial.print(humidity); Serial.println(" %");
      Serial.print("Tensão da bateria: "); Serial.print(voltage); Serial.println(" V");
      delay(200);
    }

  } while (Serial);
  // Conecta ao WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");

  int wifiTimeout = 50; // timeout de ~20 segundos
  while (WiFi.status() != WL_CONNECTED && wifiTimeout-- > 0) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED, !digitalRead(LED));
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());

    // Chama sua função para enviar dados para a planilha do Google
    escreverEmLista("Placa com erro no WiFi", 3, new float[] {temp , humidity, voltage});

    // Aguarda um pouco para garantir que os dados foram enviados
    delay(2000);
  } else {
    ESP.restart();
    Serial.println("\nFalha ao conectar ao WiFi!");
  }

  // Desconecta o WiFi para economizar energia
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Configura o próximo despertar
  uint64_t sleep_us = SLEEP_MINUTES * 60 * 1000000ULL; // minutos para microssegundos
  esp_sleep_enable_timer_wakeup(sleep_us);

  Serial.println("Entrando em deep sleep...");
  delay(100); // Pequeno atraso para garantir que a serial envie tudo

  // Entra em deep sleep
  esp_deep_sleep_start();
}

void loop() {
  // Nunca será executado devido ao deep sleep
}

String seguirRedirecionamento(String url) {
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
    String newUrl = http.getLocation();
    http.end();
    return seguirRedirecionamento(newUrl); // Segue o redirecionamento recursivamente
  } else {
    String response = http.getString();
    http.end();
    return response;
  }
}

void escreverEmLista(String identificacao, int numDados, float dados[]) {
  String url = googleScriptURL;
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["action"] = "escreverEmLista";
  jsonDoc["identificacao"] = identificacao;
  JsonArray jsonDados = jsonDoc.createNestedArray("dados");
  for (int i = 0; i < numDados; i++) {
    jsonDados.add(dados[i]);
  }

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  int httpResponseCode = http.POST(jsonString);
  if (httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY || httpResponseCode == HTTP_CODE_FOUND) {
    String newUrl = http.getLocation();
    http.end();
    seguirRedirecionamento(newUrl);
    Serial.println("Dados enviados");
  } else if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.println("Erro ao enviar dados");
  }
  http.end();
}

void escreverEmCelula(String identificacao, String celula, String dado) {
  String url = googleScriptURL;
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["action"] = "escreverEmCelula";
  jsonDoc["identificacao"] = identificacao;
  jsonDoc["celula"] = celula;
  jsonDoc["dado"] = dado;

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  int httpResponseCode = http.POST(jsonString);
  if (httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY || httpResponseCode == HTTP_CODE_FOUND) {
    String newUrl = http.getLocation();
    http.end();
    seguirRedirecionamento(newUrl);
    Serial.println("Dados enviados");
  } else if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.println("Erro ao enviar dados");
  }
  http.end();
}

String lerCelula(String identificacao, String celula) {
  String url = googleScriptURL;
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["action"] = "lerCelula";
  jsonDoc["identificacao"] = identificacao;
  jsonDoc["celula"] = celula;

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  int httpResponseCode = http.POST(jsonString);
  if (httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY || httpResponseCode == HTTP_CODE_FOUND) {
    String newUrl = http.getLocation();
    http.end();
    return seguirRedirecionamento(newUrl);
  } else if (httpResponseCode > 0) {
    String response = http.getString();
    http.end();
    return response;
  } else {
    http.end();
    return "Erro ao ler célula";
  }
}

String lerLinha(String identificacao, int linha) {
  String url = googleScriptURL;
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["action"] = "lerLinha";
  jsonDoc["identificacao"] = identificacao;
  jsonDoc["linha"] = linha;

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  int httpResponseCode = http.POST(jsonString);
  if (httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY || httpResponseCode == HTTP_CODE_FOUND) {
    String newUrl = http.getLocation();
    http.end();
    return seguirRedirecionamento(newUrl);
  } else if (httpResponseCode > 0) {
    String response = http.getString();
    http.end();
    return response;
  } else {
    http.end();
    return "Erro ao ler linha";
  }
}
