#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 110
#define DATA_PIN 4
#define WIDTH 10
#define HEIGHT 11
#define BRIGHTNESS 150

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

  drawRectangleBorder(posX, posY, 3, 3, strip.Color(255, 0, 0)); 
  strip.show();
  delay(200);
  strip.clear();
  posX = posX + random(-1, 2);
  posY = posY + random(-1, 2);
  Serial.print(posX);
  Serial.print(" ");
  Serial.println(posY);

  if (posX < 0)
    posX = 0;
  if (posY < 0)
    posY = 0;
  if (posX >= WIDTH-3)
    posX = WIDTH-3;
  if (posY >= HEIGHT-3)
    posY = HEIGHT-3;
    
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
