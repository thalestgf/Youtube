#include "esp_sleep.h"
#include "driver/gpio.h"
#include <WiFi.h>

const char *ssid = "REDE";
const char *password = "SENHA";

#define pino_acordar 2
#define led 8

void setup() {

  pinMode(led, OUTPUT);
  pinMode(pino_acordar, INPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(led, !digitalRead(led));
  }

  digitalWrite(led, LOW);
  delay(2000);
  digitalWrite(led, HIGH);
  
  uint64_t sleep_us = 30 * 1000000; // minutos para microssegundos
  esp_sleep_enable_timer_wakeup(sleep_us);

  esp_deep_sleep_start();

}

void loop() {

}
