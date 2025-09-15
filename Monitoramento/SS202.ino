#include <ESP8266WiFi.h>      // Inclui a biblioteca para controle Wi-Fi no ESP8266
#include <WiFiUdp.h>          // Inclui a biblioteca para comunicação UDP
#include <SoftwareSerial.h>
#include <ESP8266WebServer.h>  // Biblioteca para o servidor HTTP

// Variáveis globais de configuração
char ssid[32] = "Chapolim";         // Nome da rede Wi-Fi
char password[32] = "chapolim2014"; // Senha da rede Wi-Fi
IPAddress localIP(192, 168, 2, 108); // IP estático do ESP8266
IPAddress gateway(192, 168, 2, 1);   // Gateway (roteador)
IPAddress subnet(255, 255, 255, 0);  // Máscara de sub-rede
char udpAddress[16] = "192.168.2.100"; // IP do servidor UDP
int udpPort = 6070;                   // Porta UDP do servidor

WiFiUDP udp;                          // Cria um objeto UDP
ESP8266WebServer server(80);          // Instância do servidor HTTP na porta 80

int relayPin = D0;                    // Pino onde o relé está conectado
int d1Pin = D1;                       // Pino D1
int speakerPin = D2;                  // Pino do speaker
bool lastRelayState = LOW;            // Estado anterior do relé
bool currentRelayState = LOW;         // Estado atual do relé
bool isPacketSent = false;            // Flag para evitar duplicação de pacotes

void setup() {
  Serial.begin(115200);               // Inicia a comunicação serial para debug

  WiFi.config(localIP, gateway, subnet); // Configura o IP estático
  WiFi.begin(ssid, password);         // Conecta ao Wi-Fi

  while (WiFi.status() != WL_CONNECTED) { // Aguarda a conexão com a rede Wi-Fi
    delay(500);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");

  udp.begin(udpPort);                 // Inicia o objeto UDP na porta especificada
  pinMode(relayPin, INPUT);           // Configura o pino do relé como entrada
  pinMode(d1Pin, OUTPUT);             // Configura o pino D1 como saída
  pinMode(speakerPin, OUTPUT);        // Configura o pino D2 (speaker) como saída

  // Rota para acionar o D1
  server.on("/d1_activar", HTTP_GET, []() {
    digitalWrite(d1Pin, HIGH);  // Aciona o pino D1
    tone(speakerPin, 600, 2000);  // Emite som no pino D2 (speaker)

    // Envia o pacote UDP
    String message = "Liberação por botoeira virtual";
    udp.beginPacket(udpAddress, udpPort);
    udp.print(message);
    udp.endPacket();
    Serial.println("Mensagem UDP enviada: Liberação por botoeira virtual");

    // Responde ao cliente HTTP
    server.send(200, "application/json", "{\"message\": \"D1 acionado com sucesso\"}");
    delay(1000);
    digitalWrite(d1Pin, LOW);  // Desliga o pino D1 após 1 segundo
  });

  server.begin();  // Inicia o servidor HTTP
}

void loop() {
  server.handleClient();  // Processa as requisições HTTP

  // Verifica a conexão Wi-Fi e reconecta se necessário
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado! Tentando reconectar...");
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid, password);
  }

  // Lê o estado atual do pino do relé
  currentRelayState = digitalRead(relayPin);

  // Detecta a transição de LOW para HIGH e aciona o bip longo
  if (currentRelayState == HIGH && lastRelayState == LOW && !isPacketSent) {
    String message = "SA 202: Acesso Liberado";
    udp.beginPacket(udpAddress, udpPort);
    udp.print(message);
    udp.endPacket();
    Serial.println("Mensagem UDP enviada: Acesso Liberado");
    isPacketSent = true;

    // Gerar um bip longo
    tone(speakerPin, 1000, 2000);  // Frequência de 1 kHz por 1 segundo
  }

  // Detecta a transição de HIGH para LOW e reseta a flag para o próximo envio
  if (currentRelayState == LOW && lastRelayState == HIGH) {
    isPacketSent = false;
  }

  // Atualiza o estado anterior para o próximo ciclo
  lastRelayState = currentRelayState;
}
