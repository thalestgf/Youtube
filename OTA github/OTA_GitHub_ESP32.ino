#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// ===================== CONFIGURAÇÕES =====================

const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// Repositório no formato "usuario/repositorio"
const char* GITHUB_REPO = "thalestgf/OTA_github_teste";

// Nome exato do asset .bin que você sobe em cada release
// (ex: se você nomear sempre "firmware.bin" fica mais fácil)
const char* FIRMWARE_ASSET_NAME = "firmware.bin";

// Token só é necessário se o repositório for PRIVADO
// Para repo público, deixe como "" (string vazia)
const char* GITHUB_TOKEN = "";

// Versão atual gravada neste firmware.
// IMPORTANTE: precisa bater com o "tag_name" da release no GitHub
// (ex: se a tag da release é "v1.2.0", aqui deve ser "v1.2.0")
#define FIRMWARE_VERSION "v1.0.0"

// Intervalo entre verificações de atualização (ms)
const unsigned long CHECK_INTERVAL = 60UL * 60UL * 1000UL; // 1 hora

// ===========================================================

WiFiClientSecure secureClient;

void conectarWiFi() {
  Serial.printf("Conectando ao WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());
}

// Faz a requisição para a API do GitHub e retorna a URL de download
// do asset + a tag da release mais recente.
// Retorna true se encontrou uma versão diferente da atual.
bool verificarAtualizacao(String &downloadUrl, String &novaVersao) {
  HTTPClient https;

  // ATENÇÃO: setInsecure() ignora validação de certificado TLS.
  // É a forma mais simples para tutorial/teste, mas em produção
  // o ideal é usar um certificado root (DigiCert Global Root, que
  // é o usado pela GitHub e pela CDN de assets) fixado no código.
  secureClient.setInsecure();

  String url = String("https://api.github.com/repos/") + GITHUB_REPO + "/releases/latest";

  Serial.printf("Consultando: %s\n", url.c_str());

  if (!https.begin(secureClient, url)) {
    Serial.println("Falha ao iniciar conexão HTTPS");
    return false;
  }

  https.addHeader("User-Agent", "ESP32-OTA-Client"); // GitHub exige User-Agent
  https.addHeader("Accept", "application/vnd.github+json");

  if (strlen(GITHUB_TOKEN) > 0) {
    https.addHeader("Authorization", String("Bearer ") + GITHUB_TOKEN);
  }

  int httpCode = https.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Erro na requisição: %d\n", httpCode);
    https.end();
    return false;
  }

  // Filtro para pegar só os campos que interessam e economizar RAM,
  // já que o JSON de uma release do GitHub é bem grande.
  JsonDocument filter;
  filter["tag_name"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(
      doc, https.getStream(), DeserializationOption::Filter(filter));

  https.end();

  if (err) {
    Serial.printf("Falha ao parsear JSON: %s\n", err.c_str());
    return false;
  }

  const char* tag = doc["tag_name"];
  if (!tag) {
    Serial.println("Não encontrei tag_name na resposta");
    return false;
  }

  Serial.printf("Versão atual: %s | Última release: %s\n", FIRMWARE_VERSION, tag);

  if (String(tag) == String(FIRMWARE_VERSION)) {
    Serial.println("Firmware já está atualizado.");
    return false;
  }

  // Procura o asset com o nome configurado dentro da release
  for (JsonObject asset : doc["assets"].as<JsonArray>()) {
    const char* nome = asset["name"];
    if (nome && String(nome) == String(FIRMWARE_ASSET_NAME)) {
      downloadUrl = asset["browser_download_url"].as<String>();
      novaVersao = String(tag);
      return true;
    }
  }

  Serial.printf("Asset '%s' não encontrado na release %s\n", FIRMWARE_ASSET_NAME, tag);
  return false;
}

// Baixa o .bin da URL informada e grava via Update.h
bool baixarEAtualizar(const String &url) {
  HTTPClient https;
  secureClient.setInsecure();

  Serial.printf("Baixando firmware de: %s\n", url.c_str());

  // Segue redirecionamento (o browser_download_url do GitHub redireciona
  // para objects.githubusercontent.com)
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!https.begin(secureClient, url)) {
    Serial.println("Falha ao iniciar download");
    return false;
  }

  https.addHeader("User-Agent", "ESP32-OTA-Client");

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Erro no download: %d\n", httpCode);
    https.end();
    return false;
  }

  int tamanho = https.getSize();
  if (tamanho <= 0) {
    Serial.println("Tamanho do arquivo inválido");
    https.end();
    return false;
  }

  if (!Update.begin(tamanho)) {
    Serial.printf("Não há espaço suficiente para OTA: %s\n", Update.errorString());
    https.end();
    return false;
  }

  Serial.println("Gravando firmware...");
  WiFiClient *stream = https.getStreamPtr();
  size_t escrito = Update.writeStream(*stream);

  if (escrito != (size_t)tamanho) {
    Serial.printf("Escrita incompleta: %d de %d bytes\n", escrito, tamanho);
    https.end();
    return false;
  }

  if (!Update.end()) {
    Serial.printf("Erro ao finalizar update: %s\n", Update.errorString());
    https.end();
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("Update não finalizou corretamente");
    https.end();
    return false;
  }

  Serial.println("OTA concluído com sucesso! Reiniciando...");
  https.end();
  return true;
}

void tarefaVerificacaoOTA() {
  String downloadUrl, novaVersao;

  if (verificarAtualizacao(downloadUrl, novaVersao)) {
    Serial.printf("Nova versão disponível: %s\n", novaVersao.c_str());
    if (baixarEAtualizar(downloadUrl)) {
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("Falha na atualização, mantendo firmware atual.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("\nFirmware versão: %s\n", FIRMWARE_VERSION);

  conectarWiFi();


  // Verifica uma vez no boot
  tarefaVerificacaoOTA();
}

unsigned long ultimaChecagem = 0;

void loop() {
  // ... seu código normal aqui ...

  if (millis() - ultimaChecagem > CHECK_INTERVAL) {
    ultimaChecagem = millis();
    tarefaVerificacaoOTA();
  }
}
