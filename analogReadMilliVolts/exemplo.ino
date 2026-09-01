#define PINO_ADC 0  // Ajuste para o pino do seu ESP32 (34 para ESP32 normal, 3 para C3)
#define NUM_LEITURAS 100

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("ADC\tCalculo_Manual(mV)\tCalibrado_Media(mV)");
}

void loop() {
  uint32_t soma_raw = 0;
  uint32_t soma_calibrada = 0;

  for (int i = 0; i < NUM_LEITURAS; i++) {
    soma_raw += analogRead(PINO_ADC);
    soma_calibrada += analogReadMilliVolts(PINO_ADC);
    delay(2); // pequeno intervalo entre leituras, ajuda a evitar leituras muito correlacionadas
  }

  uint32_t media_raw = soma_raw / NUM_LEITURAS;
  uint32_t media_calibrada = soma_calibrada / NUM_LEITURAS;
  uint32_t tensao_manual = (media_raw * 3300) / 4095;

  Serial.print(media_raw);
  Serial.print("\t\t");
  Serial.print(tensao_manual);
  Serial.print("\t\t\t");
  Serial.println(media_calibrada);

  delay(500);
}
