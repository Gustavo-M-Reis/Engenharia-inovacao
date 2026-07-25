//------------------------------------------------------------------// 
//                                                                  //
//    Soluções Para Desafios em Engenharia - Garrafa Inteligente    //
//                                                                  //
//------------------------------------------------------------------//

#include "MPU6050.h"
#include <Wire.h>
#include "Adafruit_SSD1306.h"

#define CHARGE_PIN 10
#define SENSOR_PIN A0
#define BAT_SENS A7
#define BUZZER_PIN 6
#define BOT_CIMA 2
#define BOT_BAIXO 4
#define BOT_CONFIRMA 3
#define LED 5
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define TEMPO_DEBOUNCE 100-1
#define TEMPO_ESPERA 5000-1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MPU6050 IMU;

// Definição da CLASSE Timer

class Timer{
    public:
    unsigned long last_time;
    Timer::Timer(){
        last_time = millis();
    }
    void Timer::reset(){
        last_time = millis();
    }
    unsigned long Timer::get(){
        return millis() - last_time;
    }
};

struct Botao {
  uint8_t botao;
  bool estadoAnterior;
  unsigned long ultimoTempo;
};

enum STATE {HELLO, IDADE, PESO, SAVE, PRINCIPAL, AVISO};

// Declaração das variáveis globais

struct Botao BOT_UP = {BOT_CIMA, 0, 0};     // Botão +
struct Botao BOT_DW = {BOT_BAIXO, 0, 0};    // Botão -
struct Botao BOT_OK = {BOT_CONFIRMA, 0, 0}; // Botão Confirma
bool B_UP = 0;
bool B_DW = 0;
bool B_OK = 0;
bool estavel = 1;                  // Está estável? 0 = não / 1 = sim
uint16_t idade = 60;               // Idade inicializa com 60 anos - produto focado em idosos 
uint16_t peso = 70;                // Peso inicializa com 70 Kg
uint16_t consumo_est = 0;          // Consumo estimado - calculo com base em peso e idade
uint16_t consumo_atual = 0;        // Consumo atual - zera quando a garrafa é inicializada
uint16_t bateria = 0;              // Valor em porcentagem da bateria = bat_atual/bat_max
unsigned long tempo_ref;           // Tempo desde de a inicialização 
unsigned long tempo_deb = 50;      // Tempo para debouncing 
enum STATE modo_display = HELLO;       
enum STATE modo_sys = HELLO;

// Declaração das Funções

// Funções - Igor
uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin);
float adjust(float input, float input_min, float input_max, float output_min, float output_max);
void buzzer_logic(byte state);
void move_detect();

// Funções - Lucas
bool ler_botoes(Botao *btn);     // Lê os botões e implementa debouncing
bool esta_estavel();             // Verifica se está estável para leitura
uint16_t quanto_bebeu();         // Lê a variação de água e incrementa o consumo
uint16_t carga_bateria();        // Devolve a porcentagem da bateria
void controle_sys();             // Implementa máquina de estados do sistema
void controle_display();         // Implementa máquina de estados do display
uint16_t consumo_estimado();     // Calcula o consumo estimado baseado na idade e peso

void setup() {
  // Setup do arquivo de teste - talvez precise de ajustes
    Serial.begin(115200);
    pinMode(CHARGE_PIN, OUTPUT);
    digitalWrite(CHARGE_PIN, LOW);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    pinMode(BOT_CIMA, INPUT_PULLUP);
    pinMode(BOT_BAIXO, INPUT_PULLUP);
    pinMode(BOT_CONFIRMA, INPUT_PULLUP);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while(1);
    }
    if(IMU.begin()){
        Serial.println("erro ao iniciar a MPU");
        while(1);
    }
    move_detect();
    Timer Timer_LED;
    Timer Timer_IMU;
    int z_accel = 1;

    tempo_ref = millis();      // Inicializa o tempo de referência no final do SETUP
}

void loop() { 
  // Chama as funções necessárias no LOOP - pode precisar de alterações
  B_UP = ler_botoes(&BOT_UP);
  B_DW = ler_botoes(&BOT_DW);
  B_OK = ler_botoes(&BOT_OK);
  
  controle_sys();
  controle_display();
}

// A partir daqui são definidas as funções chamadas no setup() e loop()

// Funções de controle/sensoriamento escritas pelo Lucas

bool ler_botoes(Botao *btn){
  bool leitura = !digitalRead(btn->botao); 
  bool clicou = 0;
  if (leitura != btn->estadoAnterior) {    
    if (leitura == 1 && (millis() - btn->ultimoTempo > TEMPO_DEBOUNCE)) {
      clicou = 1;
      btn->ultimoTempo = millis();
    }
    btn->estadoAnterior = leitura; 
  } 
  return clicou;
}

bool esta_estavel(){
  // Precisa ver como verificar se está estável
  return 1;
}

uint16_t quanto_bebeu(){
  // Precisa ver como vai fazer para obter as medidas de nível
  return 0;
}

uint16_t carga_bateria(){
  // Precisa ver como vai fazer para obter a carga máxima
  return 50;
}

uint16_t consumo_estimado(){
  uint16_t fator_ml = 0;
  if (idade <= 17) {
    fator_ml = 40;
  } 
  else if (idade <= 55) {
    fator_ml = 35;
  } 
  else if (idade <= 65) {
    fator_ml = 30;
  } 
  else {
    fator_ml = 25;
  }
  return peso * fator_ml;
}

// Máquina de Estados Finita do Sistema

void controle_sys(){
  switch(modo_sys){
    case HELLO:
      modo_display = HELLO;
      if (millis()-tempo_ref >= TEMPO_ESPERA){
        tempo_ref = millis();
        modo_sys = IDADE;
      }
      break;
    case IDADE:
      modo_display = IDADE;
      if (B_UP == 1 && idade < 150)
        idade++;
      if (B_DW == 1 && idade > 5)
        idade--;
      if (B_OK == 1)
        modo_sys = PESO;
      break;
    case PESO:
      modo_display = PESO;
      if (B_UP == 1 && peso < 150)
        peso++;
      if (B_DW == 1 && peso > 10)
        peso--;
      if (B_OK == 1){
        modo_sys = SAVE;
        tempo_ref = millis();
      }
      break;
    case SAVE:
      modo_display = SAVE;
      if (millis()-tempo_ref >= TEMPO_ESPERA){
        consumo_est = consumo_estimado();
        modo_sys = PRINCIPAL;
      }
      break;
    case PRINCIPAL:
      modo_display = PRINCIPAL;
      consumo_atual += quanto_bebeu();
      bateria = carga_bateria();
      // Falta lógica do pressionar botão por 5 segundos
      // Falta a lógica do TIMEOUT do aviso 
      break;
    case AVISO:
      modo_display = AVISO;
      // Falta ligar LEDs e BUZZER 
      if (B_OK == 1 or quanto_bebeu() >= 100){
        consumo_atual += quanto_bebeu();
        modo_sys = PRINCIPAL;
      }
      break;
  }    
}

// Máquina de Estados Finita do Display

void controle_display(){
  switch(modo_display){
    case HELLO:
      // "HELLO!"
      break;
    case IDADE:
     
      break;
    case PESO:
      
      break;
    case SAVE:
      // "Dados salvos com sucesso"
      break;
    case PRINCIPAL:
      // mostra consumo_atual / consumo_est
      // mostra bateria
      break;
    case AVISO:
      // "Beba água ou aperte OK"
      break;
  }    
}

// Essas são funções de sensoriamento escritas pelo Igor

float adjust(float input, float input_min, float input_max, float output_min, float output_max){
    return((output_max - output_min) / (input_max - input_min))  * (input - input_min) + output_min;
}

uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin){
    pinMode(sensor_pin, OUTPUT);
    digitalWrite(sensor_pin, LOW);
    digitalWrite(charge_pin, LOW);
    delay(5);
    pinMode(sensor_pin, INPUT);
    digitalWrite(charge_pin, HIGH);
    uint16_t result = analogRead(sensor_pin);
    digitalWrite(charge_pin, LOW);
    return result;
}

void buzzer_logic(byte state){
    static unsigned long timer = millis();
    static byte current_state = 0;
    if(state){
        if(millis() - timer >= 100){
            timer = millis();
            current_state = ~current_state;
            digitalWrite(BUZZER_PIN, current_state);
        }
    }
    else digitalWrite(BUZZER_PIN, LOW);
}

void move_detect(){
  // Essa função tinha um while(1) - deixei vazia por que não sei exatamente o que será feito com ela
}
