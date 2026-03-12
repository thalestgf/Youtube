#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "";
const char *password = "";

WebServer server(80);


#define rele 3

void setup() {
  Serial.begin(115200);

  delay(1000);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(rele,OUTPUT);
  digitalWrite(rele,LOW);

  server.on("/", handleRoot);
  server.on("/releon", handleReleOn);
  server.on("/releoff", handleReleOff);
    
  server.begin();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  
  String html = "<h1>Controle do Rele</h1>";
  html += "<h2>Estado: " + String(digitalRead(rele) ? "LIGADO" : "DESLIGADO") + "</h2>";
  html += "<a href='/releon'><button>LIGAR</button></a> ";
  html += "<a href='/releoff'><button>DESLIGAR</button></a>";
  server.send(200, "text/html", html);
  
}

void handleReleOn() {
  digitalWrite(rele,HIGH);
  server.sendHeader("Location", "/");
  server.send(303);  
}

void handleReleOff() {
  digitalWrite(rele,LOW);
  server.sendHeader("Location", "/");
  server.send(303);  
}
