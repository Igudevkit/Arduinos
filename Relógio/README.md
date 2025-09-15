# ⏰ Projeto: Relógio

Este projeto faz parte do repositório [Arduinos](https://github.com/Igudevkit/Arduinos).  
O objetivo é criar um **relógio digital** usando microcontroladores e, se necessário, módulos de tempo real (RTC).

---

## 📋 Descrição

- Exibição da hora em display LCD/OLED ou Serial Monitor.  
- Utilização de módulo **RTC (DS1307 / DS3231)** para manter hora precisa.  
- Possibilidade de alarmes, cronômetros ou timers.  

---

## 🛠️ Hardware sugerido

- Placa Arduino UNO / Nano.  
- Módulo RTC (DS1307 ou DS3231).  
- Display LCD 16x2, OLED ou 7 segmentos.  
- Buzzer (para alarme opcional).  
- Protoboard e jumpers.  

---

## 📦 Bibliotecas utilizadas

- `RTClib.h` (para módulos RTC).  
- `Wire.h` (para comunicação I²C).  
- `LiquidCrystal.h` ou `Adafruit_SSD1306.h` (dependendo do display).  

---

## 🚀 Como rodar

1. Abra a pasta do projeto na **Arduino IDE**.  
2. Instale as bibliotecas necessárias via **Gerenciador de Bibliotecas**.  
3. Conecte os componentes conforme o esquema.  
4. Carregue o código para a placa.  
5. Veja a hora no display ou Serial Monitor.  

---

## 📷 Exemplos de uso

- Relógio de mesa digital.  
- Timer ou despertador simples.  
- Projeto de automação que depende de hora exata.
