#include "esp_sleep.h"
#include "driver/gpio.h"
#include <WiFi.h>

const char *ssid = "REDE";
const char *password = "SENHA";

#define pino_acordar 2
#define led 8

void setup() {

  Serial.begin(115200);
  Serial.println("Iniciando");
  pinMode(led, OUTPUT);
  pinMode(pino_acordar, INPUT);

  WiFi.begin(ssid, password);

  Serial.println("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(led, !digitalRead(led));
  }

  Serial.println("WiFi ok");
  digitalWrite(led, LOW);
  delay(2000);
  digitalWrite(led, HIGH);
  
  esp_deep_sleep_enable_gpio_wakeup(1ULL << pino_acordar, ESP_GPIO_WAKEUP_GPIO_LOW); //ESP32 C3 
  //esp_sleep_enable_ext0_wakeup((gpio_num_t)pino_acordar, 0);//ESP32 e ESP32
  
  uint64_t sleep_us = 30 * 1000000;
  esp_sleep_enable_timer_wakeup(sleep_us);
  Serial.println("Dormindo...");
  esp_deep_sleep_start();

}

void loop() {

}
