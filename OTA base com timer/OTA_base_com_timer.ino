#include <WiFi.h>
#include <ArduinoOTA.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
/*
// Configurações de Wi-Fi
const char* ssid = "Celular";
const char* password = "984193310";
*/
// Configurações de Wi-Fi
const char* ssid = "Desktop_F7717477";
const char* password = "Tt08051993";

// Handler do timer do FreeRTOS
TimerHandle_t otaTimer;

// Função de callback do timer
void otaTimerCallback(TimerHandle_t xTimer) {
  ArduinoOTA.handle(); // Chama o handle do OTA
  digitalWrite(4, !digitalRead(4));
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando...");

  int contador = 0;

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao Wi-Fi...");
    contador++;
    if (contador > 20) {
      ESP.restart();
    }
  }

  // Configuração do OTA
  ArduinoOTA.setHostname("Minha Placa");
  ArduinoOTA.begin();

  Serial.println("Pronto para OTA");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  // Cria um timer do FreeRTOS
  otaTimer = xTimerCreate(
               "OTATimer",              // Nome do timer (apenas para debug)
               pdMS_TO_TICKS(100),      // Período do timer (100 ms)
               pdTRUE,                  // Timer recarregável (auto-reload)
               (void*)0,                // ID do timer (não usado aqui)
               otaTimerCallback         // Função de callback
             );

  // Inicia o timer
  if (otaTimer != NULL) {
    xTimerStart(otaTimer, 0); // Inicia o timer imediatamente
  } else {
    Serial.println("Erro ao criar o timer!");
  }

  pinMode(4, OUTPUT);

}

void loop() {


}
