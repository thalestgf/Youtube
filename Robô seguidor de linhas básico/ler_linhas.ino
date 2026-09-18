#include <Wire.h>
#include <Adafruit_ADS1X15.h>

#define m1_a  4
#define m1_b  5
#define m2_a  6
#define m2_b  7

#define limiar_1  1700
#define limiar_2  5000
#define limiar_3  5000
#define limiar_4  1700

int movimento;

Adafruit_ADS1115 ads;  // Cria objeto para o ADC1115

void setup() {
  Serial.begin(115200);

  // Inicializa comunicação I2C
  Wire.begin(8, 9);  // SDA no pino 8, SCL no pino 9

  // Inicializa o ADC1115
  if (!ads.begin(0x48, &Wire)) {  // Endereço padrão 0x48
    Serial.println("Falha ao inicializar o ADS1115");
    while (1);
  }
  // Configuração para máxima velocidade
  ads.setDataRate(RATE_ADS1115_860SPS);  // 860 amostras por segundo (máximo)
  ads.setGain(GAIN_TWOTHIRDS);   // Ganho menor = menor tempo de conversão

  pinMode(m1_a, OUTPUT);
  pinMode(m1_b, OUTPUT);
  pinMode(m2_a, OUTPUT);
  pinMode(m2_b, OUTPUT);

  char receivedChar;

  if (Serial) {
    do {
      if (Serial.available() > 0) {
        receivedChar = Serial.read();
        Serial.print("Recebido: ");
        Serial.println(receivedChar);
      }
      Serial.print("L1: ");
      Serial.print(ads.readADC_SingleEnded(3));
      Serial.print(" L2: ");
      Serial.print(ads.readADC_SingleEnded(2));
      Serial.print(" L3: ");
      Serial.print(ads.readADC_SingleEnded(1));
      Serial.print(" L4: ");
      Serial.println(ads.readADC_SingleEnded(0));

      delay(100);
    } while (receivedChar != 'a');
  }

}

void loop() {
  //Serial.println("Foi");
  bool linha1 = lerLinha(1, limiar_1);
  bool linha2 = lerLinha(2, limiar_2);
  bool linha3 = lerLinha(3, limiar_3);
  bool linha4 = lerLinha(4, limiar_4);


  if (linha2 == 1 && linha3 == 1) {
    frente(100);
    movimento = 0;
  }
  if (linha2 == 0 && linha3 == 1) {
    direita(100);
    movimento = 1;
  }
  if (linha2 == 1 && linha3 == 0) {
    esquerda(100);
    movimento = 2;
  }

  if (linha2 == 0 && linha3 == 0) {
    if (movimento == 1) {
      giraDireita(60);
      //direita(100);
    }
    if (movimento == 2) {
      giraEsquerda(60);
      //esquerda(100);
    }
    if (movimento == 0) {
      tras(60);
    }
  }


}


bool lerLinha(int linha, int16_t limiar) {
  if (linha > 0 && linha < 5) {
    int16_t adc = 0;
    adc = ads.readADC_SingleEnded((-1 * linha) + 4);
    if (adc < limiar) {
      return 1;
    }
    else {
      return 0;
    }
  }
}


void motorEsq(bool sentido, long int vel) {

  if (vel > 100) {
    vel = 100;
  }
  if (vel < 0) {
    vel = 0;
  }
  if (!sentido) {
    vel = map(vel, 0, 100, 0, 250); //map(valor, minimo_atual, maximo_atual, novo_minimo, novo_maximo
  }
  else {
    vel = map(vel, 0, 100, 250, 0);
  }
  digitalWrite(m1_a, sentido);
  analogWrite(m1_b, vel);
}
void motorDir(bool sentido, long int vel) {

  if (vel > 100) {
    vel = 100;
  }
  if (vel < 0) {
    vel = 0;
  }
  if (!sentido) {
    vel = map(vel, 0, 100, 0, 250); //map(valor, minimo_atual, maximo_atual, novo_minimo, novo_maximo
  }
  else {
    vel = map(vel, 0, 100, 250, 0);
  }

  digitalWrite(m2_a, sentido);
  analogWrite(m2_b, vel);
}

void tras(int vel) {
  motorEsq(1, vel);
  motorDir(1, vel);
}


void frente(int vel) {
  motorEsq(0, vel);
  motorDir(0, vel);
}


void direita(int vel) {
  motorEsq(0, vel);
  motorDir(0, 0);
}

void esquerda(int vel) {
  motorEsq(0, 0);
  motorDir(0, vel);
}


void giraDireita(int vel) {
  motorEsq(0, vel);
  motorDir(1, vel);
}

void giraEsquerda(int vel) {
  motorEsq(1, vel);
  motorDir(0, vel);
}

void para() {
  motorEsq(0, 0);
  motorDir(0, 0);
}
