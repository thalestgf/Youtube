/*
  ===========================================================================
  PAINEL DE LED 22x20 - CONTROLE PELO NAVEGADOR (multiplas figuras salvas)
  ESP32-C3 + FastLED + WiFiManager + WebServer + LittleFS

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
     grade 22x20 clicável pra desenhar.
  3. Agora é possível salvar VÁRIAS figuras diferentes, cada uma com um
     nome (ex: "coracao", "logo", "foguete"), guardadas no sistema de
     arquivos interno (LittleFS). Uma caixa de seleção lista todas as
     figuras salvas, permitindo carregar qualquer uma delas no painel.
  4. A última figura carregada/salva fica marcada como "atual" e volta
     sozinha na próxima vez que a placa ligar.

  IMPORTANTE - PARTICIONAMENTO:
  Esse sketch usa LittleFS, que precisa de espaço reservado pra arquivos.
  Em Tools > Partition Scheme, use uma opção que tenha SPIFFS/LittleFS
  disponível, por exemplo "Huge APP (3MB No OTA/1MB SPIFFS)".

  BIBLIOTECAS NECESSÁRIAS (Library Manager da Arduino IDE):
  - FastLED
  - WiFiManager (autor: tzapu)
  - (WebServer, LittleFS e ESPmDNS já vêm com o core do ESP32)
  ===========================================================================
*/

#include <FastLED.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ESPmDNS.h>

// ---------------------- CONFIGURAÇÃO DO HARDWARE ----------------------
#define DATA_PIN      4          // LEDs no GPIO4
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define BRIGHTNESS    1         // 0-255

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
CRGB imagem[NUM_PIXELS_LOGICOS]; // buffer lógico (linha x coluna), o que é exibido

WebServer server(80);

const char* NOME_AP = "PainelLED-Setup"; // nome da rede WiFi de configuração
const char* PASTA_FIGURAS = "/figs";      // pasta onde ficam as figuras salvas
const char* ARQUIVO_ATUAL = "/current.txt"; // guarda o nome da ultima figura exibida

String nomeFiguraAtual = ""; // nome (sem extensao) da figura atualmente no painel

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
// Sanitiza o nome digitado pelo usuário pra virar um nome de arquivo
// seguro: mantém letras/números, troca espaço/hífen por underline,
// ignora acentos e símbolos, limita o tamanho.
// ---------------------------------------------------------------------
String sanitizarNome(String nome) {
  String resultado;
  nome.trim();
  for (unsigned int i = 0; i < nome.length() && resultado.length() < 24; i++) {
    char c = nome[i];
    if (isalnum((unsigned char)c)) {
      resultado += c;
    } else if (c == ' ' || c == '-' || c == '_') {
      resultado += '_';
    }
    // qualquer outro caractere (acentos, simbolos, etc.) e ignorado
  }
  if (resultado.length() == 0) resultado = "figura";
  return resultado;
}

// ---------------------------------------------------------------------
// Persistência (LittleFS) - varias figuras nomeadas, cada uma em um
// arquivo binario dentro de /figs, + um arquivo /current.txt apontando
// pra qual figura deve ser exibida ao ligar a placa.
// ---------------------------------------------------------------------
bool salvarFigura(const String &nome) {
  String caminho = String(PASTA_FIGURAS) + "/" + nome + ".bin";
  File f = LittleFS.open(caminho, "w");
  if (!f) return false;
  size_t escrito = f.write((uint8_t*)imagem, sizeof(imagem));
  f.close();
  return (escrito == sizeof(imagem));
}

bool carregarFigura(const String &nome) {
  String caminho = String(PASTA_FIGURAS) + "/" + nome + ".bin";
  if (!LittleFS.exists(caminho)) return false;
  File f = LittleFS.open(caminho, "r");
  if (!f) return false;
  bool ok = false;
  if (f.size() == (int)sizeof(imagem)) {
    f.read((uint8_t*)imagem, sizeof(imagem));
    ok = true;
  }
  f.close();
  return ok;
}

void definirFiguraAtual(const String &nome) {
  File f = LittleFS.open(ARQUIVO_ATUAL, "w");
  if (f) {
    f.print(nome);
    f.close();
  }
  nomeFiguraAtual = nome;
}

// Retorna uma lista "nome1,nome2,nome3" com todas as figuras salvas
String listarFiguras() {
  String resultado = "";
  File dir = LittleFS.open(PASTA_FIGURAS);
  if (!dir || !dir.isDirectory()) return resultado;

  File entrada = dir.openNextFile();
  bool primeiro = true;
  while (entrada) {
    String nomeArquivo = String(entrada.name());
    int barra = nomeArquivo.lastIndexOf('/');
    if (barra >= 0) nomeArquivo = nomeArquivo.substring(barra + 1);

    if (nomeArquivo.endsWith(".bin")) {
      nomeArquivo = nomeArquivo.substring(0, nomeArquivo.length() - 4);
      if (!primeiro) resultado += ",";
      resultado += nomeArquivo;
      primeiro = false;
    }
    entrada.close();
    entrada = dir.openNextFile();
  }
  dir.close();
  return resultado;
}

// Carrega, no boot, a última figura marcada como atual (se existir)
void carregarFiguraInicial() {
  bool carregou = false;

  if (LittleFS.exists(ARQUIVO_ATUAL)) {
    File f = LittleFS.open(ARQUIVO_ATUAL, "r");
    String nome = f.readString();
    f.close();
    nome.trim();
    if (nome.length() > 0 && carregarFigura(nome)) {
      nomeFiguraAtual = nome;
      carregou = true;
      Serial.println("Figura inicial carregada: " + nome);
    }
  }

  if (!carregou) {
    for (int i = 0; i < NUM_PIXELS_LOGICOS; i++) {
      imagem[i] = CRGB::Black;
    }
    Serial.println("Nenhuma figura atual encontrada. Painel iniciara apagado.");
  }
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
  .barra {
    display:flex; flex-wrap:wrap; gap:8px; justify-content:center; align-items:center;
    margin-bottom:14px;
  }
  .paleta { width:28px; height:28px; border-radius:50%; border:2px solid #555; cursor:pointer; }
  button {
    padding:10px 16px; border:none; border-radius:6px; font-size:15px; cursor:pointer;
  }
  #btnAplicar        { background:#2b7de9; color:white; }
  #btnLimpar         { background:#e74c3c; color:white; }
  #btnSalvarComoFigura { background:#2ecc71; color:white; }
  #btnCarregarFigura   { background:#2b7de9; color:white; }
  #btnApagarFigura     { background:#e74c3c; color:white; }
  input[type=color] { width:40px; height:36px; border:none; padding:0; background:none; cursor:pointer; }
  #corPersonalizadaTexto, #nomeFiguraTexto, #seletorFiguras {
    padding:8px 10px; border-radius:6px; border:1px solid #444; background:#2a2a2a; color:#eee; font-size:13px;
  }
  #separadorCustom { width:1px; height:28px; background:#444; margin:0 4px; }
  #corPersonalizadaSwatch { border-color:#888; }
  #estadoTexto { font-size: 13px; color:#aaa; margin-top:6px; margin-bottom:16px; }
  h3 { font-size: 14px; color:#aaa; margin: 20px 0 8px; font-weight: normal; }
</style>
</head>
<body>

<h2>Painel de LED 22x20</h2>

<div class="barra">
  <input type="color" id="corAtual" value="#ff0000">
  <div class="paleta" style="background:#ff0000" data-cor="#ff0000"></div>
  <div class="paleta" style="background:#00ff00" data-cor="#00ff00"></div>
  <div class="paleta" style="background:#0000ff" data-cor="#0000ff"></div>
  <div class="paleta" style="background:#ffff00" data-cor="#ffff00"></div>
  <div class="paleta" style="background:#ffffff" data-cor="#ffffff"></div>
  <div class="paleta" style="background:#000000; border-color:#888" data-cor="#000000"></div>
  <div id="separadorCustom"></div>
  <input type="text" id="corPersonalizadaTexto" placeholder="Hex (FF6B00) ou RGB (255,107,0)" size="26">
  <div class="paleta" id="corPersonalizadaSwatch" style="background:#ff6b00" data-cor="#ff6b00" title="Cor personalizada"></div>
</div>

<div id="grade"></div>

<div class="barra">
  <button id="btnAplicar">Aplicar no painel</button>
  <button id="btnLimpar">Limpar tudo</button>
</div>

<h3>Figuras salvas</h3>
<div class="barra">
  <select id="seletorFiguras">
    <option value="">-- selecione uma figura --</option>
  </select>
  <button id="btnCarregarFigura">Carregar</button>
  <button id="btnApagarFigura">Excluir</button>
</div>
<div class="barra">
  <input type="text" id="nomeFiguraTexto" placeholder="Nome da nova figura" maxlength="24">
  <button id="btnSalvarComoFigura">Salvar como nova figura</button>
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

// --- Cor personalizada via texto (hex ou RGB) ---
function interpretarCorDigitada(texto) {
  texto = texto.trim();
  if (texto === '') return null;

  const partesRgb = texto.split(',').map(s => s.trim());
  if (partesRgb.length === 3 && partesRgb.every(p => /^\d{1,3}$/.test(p))) {
    const [r, g, b] = partesRgb.map(Number);
    if (r <= 255 && g <= 255 && b <= 255) {
      return '#' + [r, g, b].map(v => v.toString(16).padStart(2, '0')).join('');
    }
    return null;
  }

  let hex = texto.replace('#', '');
  if (/^[0-9a-fA-F]{6}$/.test(hex)) {
    return '#' + hex.toLowerCase();
  }
  if (/^[0-9a-fA-F]{3}$/.test(hex)) {
    hex = hex.split('').map(c => c + c).join('');
    return '#' + hex.toLowerCase();
  }

  return null;
}

const inputCorTexto = document.getElementById('corPersonalizadaTexto');
const swatchCustom = document.getElementById('corPersonalizadaSwatch');

function aplicarCorPersonalizada() {
  const corValida = interpretarCorDigitada(inputCorTexto.value);
  if (corValida) {
    swatchCustom.style.background = corValida;
    swatchCustom.dataset.cor = corValida;
    corAtual = corValida;
    document.getElementById('corAtual').value = corValida;
    inputCorTexto.style.borderColor = '#444';
  } else {
    inputCorTexto.style.borderColor = '#e74c3c';
  }
}

inputCorTexto.addEventListener('keydown', e => {
  if (e.key === 'Enter') aplicarCorPersonalizada();
});
inputCorTexto.addEventListener('blur', aplicarCorPersonalizada);

swatchCustom.addEventListener('click', () => {
  corAtual = swatchCustom.dataset.cor;
  document.getElementById('corAtual').value = corAtual;
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

function aplicarCsvNaGrade(csv) {
  const valores = csv.split(',');
  valores.forEach((hex, i) => {
    if (celulas[i] && hex.length >= 6) {
      celulas[i].style.background = '#' + hex;
    }
  });
}

const estadoTexto = document.getElementById('estadoTexto');

async function enviar(rota, textoStatus) {
  estadoTexto.innerText = textoStatus + '...';
  try {
    const resp = await fetch(rota, { method: 'POST', body: gerarCsv() });
    estadoTexto.innerText = resp.ok ? (textoStatus + ' - OK!') : 'Erro ao enviar.';
  } catch (e) {
    estadoTexto.innerText = 'Erro de conexao com o painel.';
  }
}

document.getElementById('btnAplicar').addEventListener('click', () => enviar('/aplicar', 'Aplicando'));

// --- Figuras salvas: listar / carregar / salvar como / excluir ---
const seletorFiguras = document.getElementById('seletorFiguras');

async function carregarListaFiguras() {
  try {
    const resp = await fetch('/listar');
    const texto = (await resp.text()).trim();
    seletorFiguras.innerHTML = '<option value="">-- selecione uma figura --</option>';
    if (texto.length > 0) {
      texto.split(',').forEach(nome => {
        const opt = document.createElement('option');
        opt.value = nome;
        opt.textContent = nome;
        seletorFiguras.appendChild(opt);
      });
    }
  } catch (e) {
    // silencioso: se falhar, a lista so fica vazia
  }
}

document.getElementById('btnCarregarFigura').addEventListener('click', async () => {
  const nome = seletorFiguras.value;
  if (!nome) return;
  estadoTexto.innerText = 'Carregando figura...';
  try {
    const resp = await fetch('/carregar?nome=' + encodeURIComponent(nome), { method: 'POST' });
    if (resp.ok) {
      const csv = await resp.text();
      aplicarCsvNaGrade(csv);
      estadoTexto.innerText = 'Figura "' + nome + '" carregada.';
    } else {
      estadoTexto.innerText = 'Erro ao carregar figura.';
    }
  } catch (e) {
    estadoTexto.innerText = 'Erro de conexao.';
  }
});

document.getElementById('btnApagarFigura').addEventListener('click', async () => {
  const nome = seletorFiguras.value;
  if (!nome) return;
  if (!confirm('Excluir a figura "' + nome + '"? Essa acao nao pode ser desfeita.')) return;
  try {
    const resp = await fetch('/apagar?nome=' + encodeURIComponent(nome), { method: 'POST' });
    if (resp.ok) {
      estadoTexto.innerText = 'Figura excluida.';
      await carregarListaFiguras();
    } else {
      estadoTexto.innerText = 'Erro ao excluir.';
    }
  } catch (e) {
    estadoTexto.innerText = 'Erro de conexao.';
  }
});

document.getElementById('btnSalvarComoFigura').addEventListener('click', async () => {
  const nomeInput = document.getElementById('nomeFiguraTexto');
  const nome = nomeInput.value.trim();
  if (!nome) { alert('Digite um nome para a figura.'); return; }
  estadoTexto.innerText = 'Salvando figura...';
  try {
    const resp = await fetch('/salvarComo?nome=' + encodeURIComponent(nome), { method: 'POST', body: gerarCsv() });
    if (resp.ok) {
      estadoTexto.innerText = 'Figura "' + nome + '" salva!';
      nomeInput.value = '';
      await carregarListaFiguras();
    } else {
      estadoTexto.innerText = 'Erro ao salvar figura.';
    }
  } catch (e) {
    estadoTexto.innerText = 'Erro de conexao.';
  }
});

// carrega o estado atual do painel e a lista de figuras ao abrir a pagina
fetch('/estado').then(r => r.text()).then(csv => {
  aplicarCsvNaGrade(csv);
  estadoTexto.innerText = 'Estado atual carregado.';
}).catch(() => {
  estadoTexto.innerText = 'Nao foi possivel carregar o estado atual.';
});

carregarListaFiguras();
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

void handleListar() {
  server.send(200, "text/plain", listarFiguras());
}

void handleSalvarComo() {
  String corpo = server.arg("plain");
  String nome = sanitizarNome(server.arg("nome"));

  if (!aplicarCsvNaImagem(corpo)) {
    server.send(400, "text/plain", "Dados invalidos");
    return;
  }

  atualizarLeds();

  if (salvarFigura(nome)) {
    definirFiguraAtual(nome);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(500, "text/plain", "Erro ao salvar no LittleFS");
  }
}

void handleCarregar() {
  String nome = sanitizarNome(server.arg("nome"));

  if (carregarFigura(nome)) {
    atualizarLeds();
    definirFiguraAtual(nome);
    server.send(200, "text/plain", gerarCsvDaImagem());
  } else {
    server.send(404, "text/plain", "Figura nao encontrada");
  }
}

void handleApagar() {
  String nome = sanitizarNome(server.arg("nome"));
  String caminho = String(PASTA_FIGURAS) + "/" + nome + ".bin";

  if (LittleFS.exists(caminho)) {
    LittleFS.remove(caminho);
    if (nome == nomeFiguraAtual) {
      LittleFS.remove(ARQUIVO_ATUAL);
      nomeFiguraAtual = "";
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(404, "text/plain", "Figura nao encontrada");
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

  // --- Sistema de arquivos (LittleFS) ---
  if (!LittleFS.begin(true)) { // true = formata automaticamente se nao houver filesystem valido
    Serial.println("Falha ao montar LittleFS!");
  }
  if (!LittleFS.exists(PASTA_FIGURAS)) {
    LittleFS.mkdir(PASTA_FIGURAS);
  }

  // --- Carrega a ultima figura marcada como atual (ou apaga tudo) ---
  carregarFiguraInicial();
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
  server.on("/listar", HTTP_GET, handleListar);
  server.on("/salvarComo", HTTP_POST, handleSalvarComo);
  server.on("/carregar", HTTP_POST, handleCarregar);
  server.on("/apagar", HTTP_POST, handleApagar);
  server.begin();
  Serial.println("Servidor web iniciado.");
}

// ---------------------------------------------------------------------
// LOOP
// ---------------------------------------------------------------------
void loop() {
  server.handleClient();
}
