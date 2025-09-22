/*
  Relógio ESP32-S2 SuperMini com:
  - Display OLED SSD1306 (I2C)
  - RTC DS3231
  - Sincronização NTP via WiFi
  - Vibra Call para alarme
  - BMP280 para altitude/temperatura
  - Menu via botões físicos
  - Cronômetro e Alarme funcionando
  - Modo Deep Sleep
*/

// -------------------- Bibliotecas --------------------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_BMP280.h>

// -------------------- Definições --------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// Botões
#define SDA_PIN 1  // GPIO1 -> SDA do I²C
#define SCL_PIN 2  // GPIO2 -> SCL do I²C
#define MODE_PIN 3
#define SET_PIN  4
#define UP_PIN   5
#define SLEEP_PIN 6 // Botão deep sleep

// Vibra call
// -------------------- Vibra Call --------------------
#define VIBRA_PIN 10 // GPIO10
#define PWM_CHANNEL 0   // Canal PWM (0–15)
#define PWM_FREQ 2000   // Frequência em Hz (2 kHz está ótimo pro vibra call)
#define PWM_RESOLUTION 8 // Resolução (8 bits -> 0-255 duty cycle)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;
Adafruit_BMP280 bmp;

// WiFi e NTP
const char* ssid     = "SeuWiFi";
const char* password = "SuaSenha";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

// -------------------- Variáveis de Menu --------------------
enum Menu { RELÓGIO, CRONOMETRO, ALARME };
Menu menuAtual = RELÓGIO;

// Cronômetro
unsigned long cronometroStart = 0;
bool cronometroRodando = false;

// Alarme
int alarmeHora = 7;    
int alarmeMinuto = 0;  
bool alarmeAtivo = true;

// Controle de botões
bool modePressed = false;
bool setPressed = false;
bool sleepPressed = false;

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // Configura PWM no pino do vibra call
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(VIBRA_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0); // começa desligado

  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(SET_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(SLEEP_PIN, INPUT_PULLUP); // Botão deep sleep

  // Inicializa I2C nos pinos corretos do ESP32-S2 SuperMini
 Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializa Display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha ao iniciar OLED!");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Inicializa RTC
  if (!rtc.begin()) {
    Serial.println("RTC não encontrado!");
    while (1);
  }

  // Inicializa BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("Falha ao iniciar BMP280!");
    while (1);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  // Conecta WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");
  timeClient.begin();
}

// -------------------- Loop --------------------
void loop() {
  // ---------- Verifica botão deep sleep ----------
  if (digitalRead(SLEEP_PIN) == LOW && !sleepPressed) {
    sleepPressed = true;
    Serial.println("Entrando em Deep Sleep...");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Deep Sleep Ativado");
    display.display();
    delay(500); // Mostra mensagem

    // Configura wakeup para o mesmo botão
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_PIN, 0);
    esp_deep_sleep_start();
  }
  if (digitalRead(SLEEP_PIN) == HIGH) sleepPressed = false;

  // ---------- Atualiza NTP e RTC ----------
  timeClient.update();
  DateTime now = rtc.now();
  DateTime ntpTime = DateTime(timeClient.getEpochTime());
  long diff = (long)now.unixtime() - (long)ntpTime.unixtime();
  if (abs(diff) > 5) rtc.adjust(ntpTime);
  now = rtc.now();

  // ---------- Lê sensores ----------
  float altitude = bmp.readAltitude(1013.25);
  float temperature = bmp.readTemperature();

  // ---------- Leitura dos botões ----------
  if (digitalRead(MODE_PIN) == LOW && !modePressed) {
    menuAtual = (Menu)((menuAtual + 1) % 3);
    modePressed = true;
  }
  if (digitalRead(MODE_PIN) == HIGH) modePressed = false;

  if (digitalRead(SET_PIN) == LOW && !setPressed) {
    if (menuAtual == CRONOMETRO) {
      cronometroRodando = !cronometroRodando;
      if (cronometroRodando) cronometroStart = millis();
    } else if (menuAtual == ALARME) {
      alarmeAtivo = !alarmeAtivo;
    }
    setPressed = true;
  }
  if (digitalRead(SET_PIN) == HIGH) setPressed = false;

  if (digitalRead(UP_PIN) == LOW) {
    if (menuAtual == ALARME) {
      alarmeMinuto++;
      if (alarmeMinuto > 59) {
        alarmeMinuto = 0;
        alarmeHora = (alarmeHora + 1) % 24;
      }
      delay(200); // Debounce
    }
  }

  // ---------- Verifica Alarme ----------
if (alarmeAtivo && now.hour() == alarmeHora && now.minute() == alarmeMinuto && now.second() == 0) {
  ledcWrite(PWM_CHANNEL, 180); // vibra forte (0–255)
} else {
  ledcWrite(PWM_CHANNEL, 0);   // desligado
}


  // ---------- Atualiza Display ----------
  display.clearDisplay();
  display.setCursor(0,0);

  if (menuAtual == RELÓGIO) {
    display.print("Hora: ");
    display.print(now.hour());
    display.print(":");
    display.print(now.minute());
    display.print(":");
    display.println(now.second());

    display.print("Data: ");
    display.print(now.day());
    display.print("/");
    display.print(now.month());
    display.print("/");
    display.println(now.year());

    display.print("Alt: ");
    display.print(altitude);
    display.println(" m");

    display.print("Temp: ");
    display.print(temperature);
    display.println(" C");
  }
  else if (menuAtual == CRONOMETRO) {
    display.println("Cronometro:");
    unsigned long tempo = cronometroRodando ? millis() - cronometroStart : 0;
    unsigned int segundos = (tempo / 1000) % 60;
    unsigned int minutos = (tempo / 60000) % 60;
    unsigned int horas = tempo / 3600000;
    display.print("Tempo: ");
    display.print(horas);
    display.print(":");
    display.print(minutos);
    display.print(":");
    display.println(segundos);
  }
  else if (menuAtual == ALARME) {
    display.println("Ajuste do Alarme:");
    display.print("Ativo: ");
    display.println(alarmeAtivo ? "SIM" : "NAO");
    display.print("Hora: ");
    display.print(alarmeHora);
    display.print(":");
    display.println(alarmeMinuto);
    display.println("UP -> Ajusta Minuto/Hora");
    display.println("SET -> Liga/Desliga Alarme");
  }

  display.display();
  delay(200);
}
