#include "INA226.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define Imax 0.5 //500mA corrente máxima

#define rst_12v   PA6//Desliga a fonte 12V quando recebe nível alto
#define set_12v   PA7//Liga a fonte 12V quando recebe nível alto

#define SCL       PB6
#define SDA       PB7

// Endereços I2C - Corrigidos e organizados
#define _INA226_12V    0x44    // 0b1000100  

INA226 INA226_1(_INA226_12V);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

void setup()
{

  Serial.begin(115200);
  delay(1000);

  pinMode(set_12v, OUTPUT);
  pinMode(rst_12v, OUTPUT);

  digitalWrite(rst_12v, LOW);
  digitalWrite(set_12v, HIGH);
  delay(500);

  digitalWrite(set_12v, LOW);

  Wire.begin();
  if (!INA226_1.begin() )
  {
    Serial.println("INA226 1 Desconectado");
  }

  if (INA226_1.configure((0.033333 * 99.8 ) / 85.6, 0.05, -0.06, (10000.0 / 9.049 ) * 9.01))
    Serial.println("\n***** Erro de configuração INA226 1");


  configurarAlertaSOL(_INA226_12V, Imax * 0.03333); //Define a tensão máxima no resistor shunt e, consequentemente, a corrente máxima.

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

}

// Variáveis para as médias
float somaINA226_1 = 0;
int contadorLeituras = 0;
long int contador_fonte = 0;

void loop()
{
  // Acumula as leituras
  somaINA226_1 +=  INA226_1.getCurrent_mA();
  contadorLeituras++;

  // Quando atingir 50 leituras, calcula a média e exibe
  if (contadorLeituras >= 50) {
    // Calcula as médias
    float mediaINA219_1 = somaINA226_1 / contadorLeituras;



    // Limpa o display
    display.clearDisplay();


    //----------------Fonte 1

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("Fonte 12V");
    display.println();

    display.print("V: ");
    display.println(INA226_1.getBusVoltage(), 2);
    display.print("I: ");
    display.println(mediaINA219_1, 2);

    display.display();

    // Reinicia as variáveis para a próxima média
    somaINA226_1 = 0;
    contadorLeituras = 0;
  }

  delay(10); // Pequeno delay entre leituras
}

void configurarAlertaSOL(uint8_t ina_address, float vshunt_limite_volts) {
  // Converte limite de Vshunt para o valor do registrador (LSB = 2.5 µV)
  uint16_t alertLimitRaw = vshunt_limite_volts / 0.0000025;

  if (alertLimitRaw > 0x7FFF) alertLimitRaw = 0x7FFF; // Proteção contra overflow

  // ---- 1) Configura Mask/Enable para SOL (bit 15 = 1) ----
  Wire.beginTransmission(ina_address);
  Wire.write(0x06);                      // Registrador Mask/Enable
  Wire.write(0x80);                      // MSB com SOL = 1 (1000 0000)
  Wire.write(0x00);                      // LSB
  Wire.endTransmission();

  // ---- 2) Define limite no registrador Alert Limit (0x07) ----
  Wire.beginTransmission(ina_address);
  Wire.write(0x07);
  Wire.write(highByte(alertLimitRaw));   // MSB
  Wire.write(lowByte(alertLimitRaw));    // LSB
  Wire.endTransmission();
}

