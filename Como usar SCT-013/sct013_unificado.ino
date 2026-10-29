/*
  ===========================================================
   ESCOLHA O MODO AQUI EMBAIXO (#define MODO_CALIBRACAO):
     1 = modo CALIBRAÇÃO  (descobre a constante do seu sensor)
     0 = modo OPERAÇÃO    (mede corrente, potência e energia)
  ===========================================================

  FLUXO:
  1. Deixe MODO_CALIBRACAO em 1 e grave o código.
  2. Ligue uma carga resistiva conhecida (chuveiro, ferro, secador...)
     e meça a corrente real com um alicate amperímetro.
  3. No Serial Monitor (115200 baud, "Nova linha") digite a corrente real
     (ex: 4.35) e tecle Enter. O programa calcula a NOVA CONSTANTE.
  4. Copie a constante para CALIBRACAO_INICIAL.
  5. Mude MODO_CALIBRACAO para 0 e grave de novo. Pronto!

  Comandos no modo calibração:
    4.35     -> corrente REAL medida (calibra automaticamente)
    k 60.6   -> define a constante manualmente

  ATENÇÃO: só UM condutor (fase ou neutro) dentro do sensor!
*/

#include "EmonLib.h"

// ===================== CONFIGURAÇÃO =====================
#define MODO_CALIBRACAO 0   // 1 = calibração | 0 = operação

// Pino do sensor
const int PINO_SENSOR = 0; 


// Constante de calibração (valor inicial / valor final após calibrar)
//  - SCT-013-000 (100A/50mA) com burden de 33R -> 2000 / 33 = 60.6
//  - SCT-013-030 (30A/1V, burden interno)      -> 30.0
//  - SCT-013-020 (20A/1V, burden interno)      -> 20.0
const double CALIBRACAO_INICIAL = 21.05;

// Parâmetros de medição
const unsigned int AMOSTRAS = 1480;  // amostras por leitura do calcIrms

// Parâmetros do modo operação
const double TENSAO_REDE  = 220.0;   // 127 ou 220 V
const double LIMIAR_RUIDO = 0.10;    // abaixo disso considera 0 A
const unsigned long INTERVALO_MS = 1000;
// ========================================================

EnergyMonitor emon1;
double calibracao = CALIBRACAO_INICIAL;

// ---------- Verificação da polarização (útil nos 2 modos) ----------
void verificaPolarizacao() {
  long soma = 0;
  const int N = 2000;
  for (int i = 0; i < N; i++) soma += analogRead(PINO_SENSOR);
  double media = (double)soma / N;
#if defined(ESP32)
  double ideal = 2048;  // 12 bits
#else
  double ideal = 512;   // 10 bits
#endif
  Serial.print("Polarizacao -> ADC medio: ");
  Serial.print(media, 1);
  Serial.print("  (ideal ~ ");
  Serial.print(ideal, 0);
  Serial.println(")");
  if (abs(media - ideal) > ideal * 0.15) {
    Serial.println("ATENCAO: polarizacao fora do esperado. Confira o divisor de tensao e o capacitor!");
  }
}

// =====================================================================
//                         MODO CALIBRAÇÃO
// =====================================================================
#if MODO_CALIBRACAO

const int N_MEDIA = 10;
double buffer[N_MEDIA];
int indice = 0;
bool bufferCheio = false;

double mediaIrms() {
  int n = bufferCheio ? N_MEDIA : indice;
  if (n == 0) return 0;
  double soma = 0;
  for (int i = 0; i < n; i++) soma += buffer[i];
  return soma / n;
}

void limpaBuffer() {
  indice = 0;
  bufferCheio = false;
}

void processaComando(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  // Comando manual: k 60.6
  if (cmd.charAt(0) == 'k' || cmd.charAt(0) == 'K') {
    cmd.replace(',', '.');
    double nova = cmd.substring(1).toDouble();
    if (nova > 0) {
      calibracao = nova;
      emon1.current(PINO_SENSOR, calibracao);
      limpaBuffer();
      Serial.print(">>> Constante definida manualmente: ");
      Serial.println(calibracao, 3);
    } else {
      Serial.println(">>> Valor invalido.");
    }
    return;
  }

  // Corrente real informada
  cmd.replace(',', '.');
  double real = cmd.toDouble();
  double medido = mediaIrms();

  if (real <= 0) {
    Serial.println(">>> Valor invalido.");
    return;
  }
  if (medido < 0.05) {
    Serial.println(">>> Leitura muito baixa. Ligue a carga e confira o sensor.");
    return;
  }

  double nova = calibracao * (real / medido);
  Serial.println("--------------------------------------");
  Serial.print("Corrente real informada : "); Serial.print(real, 3);   Serial.println(" A");
  Serial.print("Media medida (antes)    : "); Serial.print(medido, 3); Serial.println(" A");
  Serial.print("Constante antiga        : "); Serial.println(calibracao, 3);
  Serial.print("NOVA CONSTANTE          : "); Serial.println(nova, 3);
  Serial.println("(copie para CALIBRACAO_INICIAL e mude MODO_CALIBRACAO para 0)");
  Serial.println("--------------------------------------");

  calibracao = nova;
  emon1.current(PINO_SENSOR, calibracao);
  limpaBuffer();
}

void modoCalibracao() {
  double irms = emon1.calcIrms(AMOSTRAS);

  buffer[indice] = irms;
  indice++;
  if (indice >= N_MEDIA) {
    indice = 0;
    bufferCheio = true;
  }

  Serial.print("Irms: ");
  Serial.print(irms, 3);
  Serial.print(" A   | Media: ");
  Serial.print(mediaIrms(), 3);
  Serial.print(" A   | Const: ");
  Serial.println(calibracao, 3);

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    processaComando(cmd);
  }
}

// =====================================================================
//                          MODO OPERAÇÃO
// =====================================================================
#else

double energiaKWh = 0;
unsigned long ultimoTempo = 0;

void modoOperacao() {
  unsigned long agora = millis();

  if (agora - ultimoTempo >= INTERVALO_MS) {
    double irms = emon1.calcIrms(AMOSTRAS);

    // Remove ruído quando não há carga
    if (irms < LIMIAR_RUIDO) irms = 0;

    double potenciaVA = irms * TENSAO_REDE;

    // Integra a energia: kW x horas
    double dtHoras = (agora - ultimoTempo) / 3600000.0;
    energiaKWh += (potenciaVA / 1000.0) * dtHoras;
    ultimoTempo = agora;

    Serial.print("Corrente: ");
    Serial.print(irms, 2);
    Serial.print(" A | Potencia: ");
    Serial.print(potenciaVA, 0);
    Serial.print(" VA | Energia: ");
    Serial.print(energiaKWh, 4);
    Serial.println(" kWh");
  }
}

#endif

// =====================================================================
//                         SETUP E LOOP
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

#if defined(ESP32)
  analogReadResolution(12);
  analogSetPinAttenuation(PINO_SENSOR, ADC_11db); // faixa ~0 a 3.3V
#endif

  emon1.current(PINO_SENSOR, calibracao);

  Serial.println();
#if MODO_CALIBRACAO
  Serial.println("=== SCT-013 | MODO CALIBRACAO ===");
  verificaPolarizacao();
  Serial.print("Constante inicial: ");
  Serial.println(calibracao, 3);
  Serial.println("Digite a corrente REAL (A) medida com o alicate e tecle Enter.");
#else
  Serial.println("=== SCT-013 | MODO OPERACAO ===");
  verificaPolarizacao();
  Serial.print("Calibracao: ");
  Serial.println(calibracao, 3);
  Serial.print("Tensao da rede: ");
  Serial.print(TENSAO_REDE, 0);
  Serial.println(" V");
  ultimoTempo = millis();
#endif
  Serial.println();
}

void loop() {
#if MODO_CALIBRACAO
  modoCalibracao();
#else
  modoOperacao();
#endif
}
