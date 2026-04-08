#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 110
#define DATA_PIN 4
#define WIDTH 10
#define HEIGHT 11
#define BRIGHTNESS 20

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

int posX = 3;
int posY = 3;

int mapping[HEIGHT][WIDTH];

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  setupMapping();  // Inicializa o mapeamento serpentina
  strip.show();
}

void loop() {
  strip.clear();

  int olho_estado = (int)random(0, 6);
  desenhaOlhosAberto(random(0, 3), olho_estado );
  for (int x = 0; x < 10 + olho_estado * 5 ; x++) {
    desenhaBoca((int)random(1, 4));
    delay(75);
  }
}



void drawRectangleBorder(int x, int y, int width, int height, uint32_t cor) {
  // Calcula o canto inferior direito
  int x2 = x + width - 1;
  int y2 = y + height - 1;

  // Garante que as coordenadas estão dentro dos limites
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x2 >= WIDTH) x2 = WIDTH - 1;
  if (y2 >= HEIGHT) y2 = HEIGHT - 1;

  // Garante que ainda tem tamanho válido
  if (x > x2 || y > y2) return;

  // Desenha a borda superior
  for (int col = x; col <= x2; col++) {
    int ledIndex = mapping[y][col];
    strip.setPixelColor(ledIndex, cor);
  }

  // Desenha a borda inferior (se diferente da superior)
  if (y2 != y) {
    for (int col = x; col <= x2; col++) {
      int ledIndex = mapping[y2][col];
      strip.setPixelColor(ledIndex, cor);
    }
  }

  // Desenha a borda esquerda (sem os cantos)
  for (int row = y + 1; row <= y2 - 1; row++) {
    int ledIndex = mapping[row][x];
    strip.setPixelColor(ledIndex, cor);
  }

  // Desenha a borda direita (sem os cantos)
  if (x2 != x) {
    for (int row = y + 1; row <= y2 - 1; row++) {
      int ledIndex = mapping[row][x2];
      strip.setPixelColor(ledIndex, cor);
    }
  }
}

// Função para desenhar retângulo preenchido (opcional)
void drawRectangleFilled(int x, int y, int width, int height, uint32_t cor) {
  int x2 = x + width - 1;
  int y2 = y + height - 1;

  x = constrain(x, 0, WIDTH - 1);
  y = constrain(y, 0, HEIGHT - 1);
  x2 = constrain(x2, 0, WIDTH - 1);
  y2 = constrain(y2, 0, HEIGHT - 1);

  for (int row = y; row <= y2; row++) {
    for (int col = x; col <= x2; col++) {
      int ledIndex = mapping[row][col];
      strip.setPixelColor(ledIndex, cor);
    }
  }
}

void drawEllipseBorder(int x, int y, int rx, int ry, uint32_t cor) {
  for (int row = 0; row < HEIGHT; row++) {
    for (int col = 0; col < WIDTH; col++) {
      int dx = col - x;
      int dy = row - y;

      int left = (dx * dx) * (ry * ry) + (dy * dy) * (rx * rx);
      int right = (rx * rx) * (ry * ry);

      // Desenha apenas os pixels próximos da borda
      if (abs(left - right) < (rx + ry) * 2) {
        int ledIndex = mapping[row][col];
        strip.setPixelColor(ledIndex, cor);
      }
    }
  }
}

// Função para configurar o mapeamento serpentina
void setupMapping() {
  int ledIndex = 0;
  for (int row = 0; row < HEIGHT; row++) {
    if (row % 2 == 0) {
      for (int col = 0; col < WIDTH; col++) {
        mapping[row][col] = ledIndex++;
      }
    } else {
      for (int col = WIDTH - 1; col >= 0; col--) {
        mapping[row][col] = ledIndex++;
      }
    }
  }
}



void desenhaOlhosAberto(int olho, bool aberto) {

  if (aberto > 0) {

    /*if (olho > 3)
      olho = 3;
      if (olho < 0)
      olho = 0;*/

    olho = olho % 3;
    
    drawRectangleFilled(1, 1, 3, 3, strip.Color(0, 0, 100));
    drawRectangleFilled(6, 1, 3, 3, strip.Color(0, 0, 100));
    drawRectangleBorder(1 + olho, 2, 1, 1, strip.Color(250, 50, 0));
    drawRectangleBorder(6 + olho, 2, 1, 1, strip.Color(250, 50, 0));
  }
  else {
    desenhaOlhosFechado();
  }

  strip.show();
}

void desenhaOlhosFechado() {


  drawRectangleFilled(1, 1, 3, 3, strip.Color(0, 0, 0));
  drawRectangleFilled(6, 1, 3, 3, strip.Color(0, 0, 0));

  drawRectangleFilled(1, 2, 3, 1, strip.Color(0, 0, 100));
  drawRectangleFilled(6, 2, 3, 1, strip.Color(0, 0, 100));
  strip.show();
}


void desenhaBoca(int abertura) {
  drawRectangleFilled(0, 4, 10, 7, strip.Color(0, 0, 0));


  drawEllipseBorder(5, 7, 3 - (int)((abertura - 1) / 3), abertura, strip.Color(0, 0, 100)); // Borda amarela

  /*
    drawRectangleFilled(2, 7, 6, 4, strip.Color(0, 0, 0));

    drawRectangleBorder(2, 7, 6, abertura, strip.Color(0, 0, 100));*/
  strip.show();
}
