#include <Preferences.h>

// Cria um objeto Preferences
Preferences preferences;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Exemplo de uso da biblioteca Preferences ===\n");

  // ===== SALVANDO DADOS =====
  Serial.println("1. Salvando dados na memória...");

  // Abre o namespace "meus_dados" em modo leitura/escrita
  // O parâmetro false significa que vamos escrever
  preferences.begin("meus_dados", false);

  preferences.clear();

  // Salva diferentes tipos de dados
  preferences.putInt("contador", 42);           // Salva um inteiro
  preferences.putFloat("temperatura", 25.5);    // Salva um float
  preferences.putString("nome", "ESP32");       // Salva uma string
  preferences.putBool("ativo", true);           // Salva um booleano

  // Salva um contador que incrementa cada vez que o código roda
  int vezesExecutado = preferences.getInt("vezes", 0) + 1;
  preferences.putInt("vezes", vezesExecutado);

  Serial.println("  Dados salvos com sucesso!");
  preferences.end(); // Fecha o namespace



  // ===== LENDO DADOS =====
  Serial.println("\n2. Lendo dados da memória...");

  // Abre o namespace novamente, agora em modo leitura
  preferences.begin("meus_dados", true); // true = modo somente leitura

  // Lê os dados salvos
  int contador = preferences.getInt("contador", 0);        // Se não existir, retorna 0
  float temperatura = preferences.getFloat("temperatura", 0.0);
  String nome = preferences.getString("nome", "desconhecido");
  bool ativo = preferences.getBool("ativo", false);
  int vezes = preferences.getInt("vezes", 0);

  // Mostra os dados lidos
  Serial.print("  Contador: ");
  Serial.println(contador);
  Serial.print("  Temperatura: ");
  Serial.println(temperatura);
  Serial.print("  Nome: ");
  Serial.println(nome);
  Serial.print("  Ativo: ");
  Serial.println(ativo ? "Sim" : "Não");
  Serial.print("  Vezes executado: ");
  Serial.println(vezes);


  Serial.println("\n=== Resetando ESP32 ===\n");
  delay(10000);

  // Restart ESP
  ESP.restart();

}

void loop() {

}
