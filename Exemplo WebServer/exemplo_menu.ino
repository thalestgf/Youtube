#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "";
const char *password = "";

WebServer server(80);

// Variáveis para armazenar os dados do formulário
String campo1 = "";
String campo2 = "";
String campo3 = "";
String opcao = "nao"; // Valor padrão

void setup() {
  Serial.begin(115200);
  
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Conectando ao WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  // Configurar rotas do servidor
  server.on("/", handleRoot);
  server.on("/salvar", HTTP_POST, handleSalvar);
  server.on("/dados", handleDados);

  server.begin();
  Serial.println("Servidor HTTP iniciado!");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Formulário ESP32-C3</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 40px; background-color: #f0f0f0; }";
  html += ".container { max-width: 500px; margin: auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += "label { display: block; margin: 15px 0 5px; color: #555; }";
  html += "input[type='text'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }";
  html += "select { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; background: white; }";
  html += "button { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; width: 100%; margin-top: 20px; font-size: 16px; }";
  html += "button:hover { background-color: #45a049; }";
  html += ".dados-salvos { margin-top: 30px; padding: 20px; background-color: #e7f3fe; border-left: 6px solid #2196F3; border-radius: 5px; }";
  html += ".dados-salvos h3 { margin-top: 0; color: #333; }";
  html += ".info-item { margin: 5px 0; }";
  html += ".info-label { font-weight: bold; color: #555; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>Formulário ESP32-C3</h1>";
  
  // Formulário
  html += "<form action='/salvar' method='POST'>";
  html += "<label for='campo1'>Campo 1:</label>";
  html += "<input type='text' id='campo1' name='campo1' value='" + campo1 + "' required>";
  
  html += "<label for='campo2'>Campo 2:</label>";
  html += "<input type='text' id='campo2' name='campo2' value='" + campo2 + "' required>";
  
  html += "<label for='campo3'>Campo 3:</label>";
  html += "<input type='text' id='campo3' name='campo3' value='" + campo3 + "' required>";
  
  html += "<label for='opcao'>Opção:</label>";
  html += "<select id='opcao' name='opcao'>";
  html += "<option value='sim' " + String(opcao == "sim" ? "selected" : "") + ">Sim</option>";
  html += "<option value='nao' " + String(opcao == "nao" ? "selected" : "") + ">Não</option>";
  html += "</select>";
  
  html += "<button type='submit'>Salvar Dados</button>";
  html += "</form>";
  
  // Exibir dados salvos
  html += "<div class='dados-salvos'>";
  html += "<h3>📋 Últimos dados salvos:</h3>";
  html += "<div class='info-item'><span class='info-label'>Campo 1:</span> " + campo1 + "</div>";
  html += "<div class='info-item'><span class='info-label'>Campo 2:</span> " + campo2 + "</div>";
  html += "<div class='info-item'><span class='info-label'>Campo 3:</span> " + campo3 + "</div>";
  html += "<div class='info-item'><span class='info-label'>Opção:</span> " + String(opcao == "sim" ? "Sim ✓" : "Não ✗") + "</div>";
  html += "</div>";
  
  // Link para página de dados
  html += "<div style='text-align: center; margin-top: 20px;'>";
  html += "<a href='/dados' style='color: #2196F3; text-decoration: none;'>Ver dados em formato JSON</a>";
  html += "</div>";
  
  html += "</div>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleSalvar() {
  if (server.hasArg("campo1") && server.hasArg("campo2") && server.hasArg("campo3") && server.hasArg("opcao")) {
    campo1 = server.arg("campo1");
    campo2 = server.arg("campo2");
    campo3 = server.arg("campo3");
    opcao = server.arg("opcao");
    
    Serial.println("Dados salvos:");
    Serial.println("Campo 1: " + campo1);
    Serial.println("Campo 2: " + campo2);
    Serial.println("Campo 3: " + campo3);
    Serial.println("Opção: " + opcao);
    
    // Redirecionar para a página principal
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(400, "text/plain", "Erro: Dados incompletos");
  }
}

void handleDados() {
  String json = "{";
  json += "\"campo1\":\"" + campo1 + "\",";
  json += "\"campo2\":\"" + campo2 + "\",";
  json += "\"campo3\":\"" + campo3 + "\",";
  json += "\"opcao\":\"" + opcao + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}
