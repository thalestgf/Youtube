/*
  ===========================================================================
  PAINEL DE LED 22x20 - CONTROLE PELO NAVEGADOR (imagem estática)
  ESP32-C3 + FastLED + WiFiManager + WebServer + Preferences (NVS)

  Mesmo hardware dos programas anteriores:
  - 4 placas de 11x10 LEDs (110 cada), em grade 2x2 (2 em cima, 2 embaixo),
    ligadas em cadeia placa1 -> placa2 -> placa3 -> placa4.
  - Matriz lógica final: 22 linhas x 20 colunas (440 LEDs).
  - Dados no GPIO4.

  O QUE ESSE PROGRAMA FAZ:
  1. Ao ligar, usa o WiFiManager para conectar na sua rede WiFi. Se não
     souber a rede (primeira vez, ou rede mudou), ele cria um Access Point
     chamado "PainelLED-Setup" - conecte o celular/PC nele, abra o navegador
     (geralmente abre sozinho - captive portal) e escolha sua rede/senha.
  2. Depois de conectado, sobe um servidor web (http://<ip-do-esp>/) com uma
     grade 22x20 clicável. Você escolhe uma cor, clica/arrasta nas células
     pra desenhar, e tem dois botões:
       - "Aplicar no painel"  -> manda o desenho pro ESP32 e acende os LEDs
                                  (não salva na memória ainda, é só um preview)
       - "Salvar na memória"  -> aplica E grava na flash (NVS). Na próxima
                                  vez que a placa ligar, essa imagem volta
                                  automaticamente.
  3. No boot, o ESP32 carrega a última imagem salva na NVS (se existir) e
     já acende o painel com ela antes mesmo de você abrir o navegador.

  BIBLIOTECAS NECESSÁRIAS (Library Manager da Arduino IDE):
  - FastLED
  - WiFiManager (autor: tzapu)
  - (WebServer, Preferences e ESPmDNS já vêm com o core do ESP32)
  ===========================================================================
*/

#include <FastLED.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>

// ---------------------- CONFIGURAÇÃO DO HARDWARE ----------------------
#define DATA_PIN      4          // LEDs no GPIO4
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define BRIGHTNESS    40         // 0-255

// ---------------------- CONFIGURAÇÃO DO PAINEL ----------------------
#define BOARD_ROWS            11
#define BOARD_COLS            10
#define BOARDS_NA_VERTICAL     2
#define BOARDS_NA_HORIZONTAL   2

#define ROWS               (BOARD_ROWS * BOARDS_NA_VERTICAL)      // 22
#define COLS               (BOARD_COLS * BOARDS_NA_HORIZONTAL)    // 20
#define LEDS_PER_BOARD     (BOARD_ROWS * BOARD_COLS)              // 110
#define NUM_BOARDS         (BOARDS_NA_VERTICAL * BOARDS_NA_HORIZONTAL) // 4
#define NUM_LEDS           (LEDS_PER_BOARD * NUM_BOARDS)          // 440
#define NUM_PIXELS_LOGICOS (ROWS * COLS)                           // 440

CRGB leds[NUM_LEDS];          // buffer físico enviado pro FastLED
CRGB imagem[NUM_PIXELS_LOGICOS]; // buffer lógico (linha x coluna), o que é salvo/carregado

WebServer server(80);
Preferences preferencias;

const char* NOME_AP = "PainelLED-Setup"; // nome da rede WiFi de configuração
const char* CHAVE_NVS_NAMESPACE = "painel";
const char* CHAVE_NVS_IMAGEM    = "img";

// ---------------------------------------------------------------------
// Mesma função de mapeamento dos programas de teste anteriores
// ---------------------------------------------------------------------
uint16_t getLedIndex(uint8_t row, uint8_t col) {
  uint8_t blocoLinha = row / BOARD_ROWS;
  uint8_t blocoCol   = col / BOARD_COLS;
  uint8_t boardIndex = blocoLinha * BOARDS_NA_HORIZONTAL + blocoCol;

  uint8_t localRow = row % BOARD_ROWS;
  uint8_t localCol = col % BOARD_COLS;

  uint16_t numeroLed;
  if (localRow % 2 == 0) {
    numeroLed = localRow * BOARD_COLS + localCol + 1;
  } else {
    numeroLed = (localRow + 1) * BOARD_COLS - localCol;
  }

  uint16_t boardOffset  = boardIndex * LEDS_PER_BOARD;
  uint16_t numeroGlobal = numeroLed + boardOffset;
  return numeroGlobal - 1;
}

// índice do array "imagem" (lógico, linha-major) pra uma coordenada (row,col)
inline uint16_t idxImagem(uint8_t row, uint8_t col) {
  return row * COLS + col;
}

// Copia o buffer lógico "imagem" pro buffer físico "leds" e manda pro painel
void atualizarLeds() {
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      leds[getLedIndex(r, c)] = imagem[idxImagem(r, c)];
    }
  }
  FastLED.show();
}

// ---------------------------------------------------------------------
// Persistência (NVS / flash) - salva e carrega o buffer "imagem" inteiro
// ---------------------------------------------------------------------
void salvarImagemNaMemoria() {
  preferencias.begin(CHAVE_NVS_NAMESPACE, false);
  size_t bytesGravados = preferencias.putBytes(CHAVE_NVS_IMAGEM, (uint8_t*)imagem, sizeof(imagem));
  preferencias.end();
  Serial.printf("Imagem salva na memoria: %u bytes\n", (unsigned)bytesGravados);
}

bool carregarImagemDaMemoria() {
  preferencias.begin(CHAVE_NVS_NAMESPACE, true);
  size_t tamanhoSalvo = preferencias.getBytesLength(CHAVE_NVS_IMAGEM);
  bool ok = false;
  if (tamanhoSalvo == sizeof(imagem)) {
    preferencias.getBytes(CHAVE_NVS_IMAGEM, (uint8_t*)imagem, sizeof(imagem));
    ok = true;
    Serial.println("Imagem carregada da memoria.");
  } else {
    Serial.println("Nenhuma imagem salva encontrada (ou tamanho incompativel). Painel iniciara apagado.");
    for (int i = 0; i < NUM_PIXELS_LOGICOS; i++) {
      imagem[i] = CRGB::Black;
    }
  }
  preferencias.end();
  return ok;
}

// ---------------------------------------------------------------------
// Converte o corpo recebido do navegador (CSV de cores hex "RRGGBB",
// 440 valores separados por virgula, ordem linha-major) pro buffer "imagem"
// ---------------------------------------------------------------------
bool aplicarCsvNaImagem(const String &csv) {
  int pos = 0;
  int indice = 0;
  int tamanho = csv.length();

  while (pos < tamanho && indice < NUM_PIXELS_LOGICOS) {
    int fimToken = csv.indexOf(',', pos);
    if (fimToken == -1) fimToken = tamanho;

    String token = csv.substring(pos, fimToken);
    token.trim();

    if (token.length() >= 6) {
      long valor = strtol(token.c_str(), nullptr, 16);
      imagem[indice].r = (valor >> 16) & 0xFF;
      imagem[indice].g = (valor >> 8) & 0xFF;
      imagem[indice].b = valor & 0xFF;
    } else {
      imagem[indice] = CRGB::Black;
    }

    indice++;
    pos = fimToken + 1;
  }

  return (indice == NUM_PIXELS_LOGICOS);
}

// Gera o CSV atual (usado pra mandar o estado pro navegador ao abrir a pagina)
String gerarCsvDaImagem() {
  String csv;
  csv.reserve(NUM_PIXELS_LOGICOS * 7);
  char buf[8];
  for (int i = 0; i < NUM_PIXELS_LOGICOS; i++) {
    sprintf(buf, "%02X%02X%02X", imagem[i].r, imagem[i].g, imagem[i].b);
    csv += buf;
    if (i < NUM_PIXELS_LOGICOS - 1) csv += ",";
  }
  return csv;
}

// ---------------------------------------------------------------------
// PÁGINA WEB (HTML + CSS + JS embutidos)
// ---------------------------------------------------------------------
const char PAGINA_HTML[] PROGMEM = R"paginaweb(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Painel de LED</title>
<style>
  body { font-family: Arial, sans-serif; background:#1e1e1e; color:#eee; text-align:center; margin:0; padding:16px; }
  h2 { margin: 8px 0 16px; }
  #grade {
    display: grid;
    grid-template-columns: repeat(20, minmax(0, 1fr));
    gap: 2px;
    max-width: 620px;
    margin: 0 auto 16px;
    aspect-ratio: 20 / 22;
    touch-action: none;
  }
  .celula {
    background:#000;
    border: 1px solid #333;
    border-radius: 2px;
    aspect-ratio: 1 / 1;
    cursor: pointer;
  }
  #barra {
    display:flex; flex-wrap:wrap; gap:8px; justify-content:center; align-items:center;
    margin-bottom:14px;
  }
  .paleta { width:28px; height:28px; border-radius:50%; border:2px solid #555; cursor:pointer; }
  button {
    padding:10px 16px; border:none; border-radius:6px; font-size:15px; cursor:pointer;
  }
  #btnAplicar { background:#2b7de9; color:white; }
  #btnSalvar  { background:#2ecc71; color:white; }
  #btnLimpar  { background:#e74c3c; color:white; }
  input[type=color] { width:40px; height:36px; border:none; padding:0; background:none; cursor:pointer; }
  #estadoTexto { font-size: 13px; color:#aaa; margin-top:6px; }
</style>
</head>
<body>

<h2>Painel de LED 22x20</h2>

<div id="barra">
  <input type="color" id="corAtual" value="#ff0000">
  <div class="paleta" style="background:#ff0000" data-cor="#ff0000"></div>
  <div class="paleta" style="background:#00ff00" data-cor="#00ff00"></div>
  <div class="paleta" style="background:#0000ff" data-cor="#0000ff"></div>
  <div class="paleta" style="background:#ffff00" data-cor="#ffff00"></div>
  <div class="paleta" style="background:#ffffff" data-cor="#ffffff"></div>
  <div class="paleta" style="background:#000000; border-color:#888" data-cor="#000000"></div>
</div>

<div id="grade"></div>

<div id="barra">
  <button id="btnAplicar">Aplicar no painel</button>
  <button id="btnSalvar">Salvar na memoria</button>
  <button id="btnLimpar">Limpar tudo</button>
</div>
<div id="estadoTexto">Carregando estado atual do painel...</div>

<script>
const LINHAS = 22, COLUNAS = 20;
let corAtual = "#ff0000";
let desenhando = false;

const grade = document.getElementById('grade');
const celulas = [];

for (let i = 0; i < LINHAS * COLUNAS; i++) {
  const c = document.createElement('div');
  c.className = 'celula';
  grade.appendChild(c);
  celulas.push(c);
}

function pintar(indice) {
  celulas[indice].style.background = corAtual;
}

grade.addEventListener('pointerdown', e => {
  if (e.target.classList.contains('celula')) {
    desenhando = true;
    pintar(celulas.indexOf(e.target));
  }
});
grade.addEventListener('pointerover', e => {
  if (desenhando && e.target.classList.contains('celula')) {
    pintar(celulas.indexOf(e.target));
  }
});
document.addEventListener('pointerup', () => desenhando = false);

document.getElementById('corAtual').addEventListener('input', e => {
  corAtual = e.target.value;
});
document.querySelectorAll('.paleta').forEach(p => {
  p.addEventListener('click', () => {
    corAtual = p.dataset.cor;
    document.getElementById('corAtual').value = corAtual;
  });
});

document.getElementById('btnLimpar').addEventListener('click', () => {
  celulas.forEach(c => c.style.background = '#000000');
});

function gerarCsv() {
  return celulas.map(c => {
    const rgb = c.style.background.match(/\d+/g) || [0,0,0];
    return rgb.map(v => parseInt(v).toString(16).padStart(2,'0')).join('').toUpperCase();
  }).join(',');
}

async function enviar(rota, textoStatus) {
  document.getElementById('estadoTexto').innerText = textoStatus + '...';
  try {
    const resp = await fetch(rota, { method: 'POST', body: gerarCsv() });
    document.getElementById('estadoTexto').innerText = resp.ok ? (textoStatus + ' - OK!') : 'Erro ao enviar.';
  } catch (e) {
    document.getElementById('estadoTexto').innerText = 'Erro de conexao com o painel.';
  }
}

document.getElementById('btnAplicar').addEventListener('click', () => enviar('/aplicar', 'Aplicando'));
document.getElementById('btnSalvar').addEventListener('click', () => enviar('/salvar', 'Salvando'));

// carrega o estado atual do painel ao abrir a pagina
fetch('/estado').then(r => r.text()).then(csv => {
  const valores = csv.split(',');
  valores.forEach((hex, i) => {
    if (celulas[i] && hex.length >= 6) {
      celulas[i].style.background = '#' + hex;
    }
  });
  document.getElementById('estadoTexto').innerText = 'Estado atual carregado.';
}).catch(() => {
  document.getElementById('estadoTexto').innerText = 'Nao foi possivel carregar o estado atual.';
});
</script>
</body>
</html>
)paginaweb";

// ---------------------------------------------------------------------
// HANDLERS DO SERVIDOR WEB
// ---------------------------------------------------------------------
void handleRaiz() {
  server.send_P(200, "text/html", PAGINA_HTML);
}

void handleEstado() {
  server.send(200, "text/plain", gerarCsvDaImagem());
}

void handleAplicar() {
  String corpo = server.arg("plain");
  if (aplicarCsvNaImagem(corpo)) {
    atualizarLeds();
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Dados invalidos");
  }
}

void handleSalvar() {
  String corpo = server.arg("plain");
  if (aplicarCsvNaImagem(corpo)) {
    atualizarLeds();
    salvarImagemNaMemoria();
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Dados invalidos");
  }
}

// ---------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);

  // --- LEDs ---
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // --- Carrega ultima imagem salva (ou apaga tudo se nao houver) ---
  carregarImagemDaMemoria();
  atualizarLeds();

  // --- WiFi via WiFiManager ---
  WiFiManager wm;
  // wm.resetSettings(); // descomente uma vez se quiser forçar reconfiguração
  wm.setConfigPortalTimeout(180); // 3 min esperando configuracao antes de tentar seguir sem WiFi

  bool conectado = wm.autoConnect(NOME_AP); // sem senha no AP; use autoConnect(NOME_AP,"senha123") p/ AP protegido

  if (!conectado) {
    Serial.println("Nao foi possivel conectar ao WiFi. Reiniciando...");
    delay(2000);
    ESP.restart();
  }

  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  // --- mDNS: permite acessar por http://painel.local ---
  if (MDNS.begin("painel")) {
    Serial.println("mDNS ativo: http://painel.local");
  }

  // --- Rotas do servidor web ---
  server.on("/", HTTP_GET, handleRaiz);
  server.on("/estado", HTTP_GET, handleEstado);
  server.on("/aplicar", HTTP_POST, handleAplicar);
  server.on("/salvar", HTTP_POST, handleSalvar);
  server.begin();
  Serial.println("Servidor web iniciado.");
}

// ---------------------------------------------------------------------
// LOOP
// ---------------------------------------------------------------------
void loop() {
  server.handleClient();
}
