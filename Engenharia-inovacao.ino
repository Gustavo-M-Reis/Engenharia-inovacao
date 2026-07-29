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
#define TEMPO_DEBOUNCE 100
#define TEMPO_ESPERA 5000
#define TEMPO_LED 500
#define TEMPO_TIMEOUT 60000 // Define o tempo de aviso como 60 segundos - apenas para teste

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

// Declaração timers

Timer Timer_LED;
Timer Timer_IMU;

// Estrutura para os botões

struct Botao {
  uint8_t botao;
  bool estadoAnterior;
  unsigned long ultimoTempo;
};

// Estados das funções de controle

enum STATE {HELLO, IDADE, PESO, SAVE, PRINCIPAL, AVISO};

// Declaração das variáveis globais

bool estavel = 1;                  // Está estável? 0 = não / 1 = sim
uint16_t idade = 60;               // Idade inicializa com 60 anos - produto focado em idosos 
uint16_t peso = 70;                // Peso inicializa com 70 Kg
uint16_t consumo_est = 0;          // Consumo estimado - calculo com base em peso e idade
uint16_t consumo_atual = 0;        // Consumo atual - zera quando a garrafa é inicializada
uint16_t bateria = 0;              // Valor em porcentagem da bateria = bat_atual/bat_max
enum STATE modo_display = HELLO;       

// Declaração das Funções

// Funções - Igor
uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin);
float adjust(float input, float input_min, float input_max, float output_min, float output_max);
void buzzer_logic(byte state);
void move_detect();

// Funções - Lucas
bool ler_botoes(Botao *btn);                             // Lê os botões e implementa debouncing
bool esta_estavel();                                     // Verifica se está estável para leitura
uint16_t quanto_bebeu();                                 // Lê a variação de água e incrementa o consumo
uint16_t carga_bateria();                                // Devolve a porcentagem da bateria
void controle_sys(bool B_UP, bool B_DW, bool B_OK);      // Implementa máquina de estados do sistema
void controle_display();                                 // Implementa máquina de estados do display
uint16_t consumo_estimado();                             // Calcula o consumo estimado baseado na idade e peso
void aviso_LED_buzzer();                                 // Controla LEDs e Buzzer
bool botao_pressionado();                                // Verifica se o botão foi pressionado por 5 segundos


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
    // Essas duas linhas tem que ver se vai precisar mesmo
    move_detect();
    int z_accel = 1;
}

void loop() { 
  // Chama as funções necessárias no LOOP - pode precisar de alterações
  static struct Botao BOT_UP = {BOT_CIMA, 0, 0};     
  static struct Botao BOT_DW = {BOT_BAIXO, 0, 0};    
  static struct Botao BOT_OK = {BOT_CONFIRMA, 0, 0};

  bool B_UP = ler_botoes(&BOT_UP);
  bool B_DW = ler_botoes(&BOT_DW);
  bool B_OK = ler_botoes(&BOT_OK);
  
  controle_sys(B_UP, B_DW, B_OK);
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

bool botao_pressionado(){
  static unsigned long tempo_pressionado = 0;
  if (!digitalRead(BOT_CONFIRMA)) { 
    if (tempo_pressionado == 0) {
      tempo_pressionado = millis(); 
      return 0;
    } 
    else if (millis() - tempo_pressionado >= 5000) {
      tempo_pressionado = 0;    
      return 1;
    }
  }
  else {
    tempo_pressionado = 0; 
    return 0;
  }  
}

bool esta_estavel(){
  // Precisa ver como verificar se está estável
  // Deve devolver 1 se está pronto para medida / 0 caso contrátio
  return 1;
}

uint16_t quanto_bebeu(){  
  // Precisa ver como vai fazer para obter as medidas de nível
  // Tem que devolver uma medida estável depois de um tempo da medição, não uma flutuação
  // ex: retornar 0 a menos que a variação lida seja maior que 10ml
  // Chamar função esta_estavel()
  // Levar em consideração essa parte do código!
  //    consumo += quanto_bebeu();
  //    if (consumo >= 50){
  //      consumo_atual += consumo;
  //      consumo = 0;
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

void aviso_LED_buzzer(){
  if(Timer_LED.get() < TEMPO_LED){
    digitalWrite(LED, HIGH);
    buzzer_logic(true);
  }
  else if(Timer_LED.get() >= TEMPO_LED && Timer_LED.get() < 2*TEMPO_LED){
    digitalWrite(LED, LOW);
    buzzer_logic(false);
  }
  else{
    Timer_LED.reset();
  }
}

// Máquina de Estados Finita do Sistema

void controle_sys(bool B_UP, bool B_DW, bool B_OK){
  static enum STATE modo_sys = HELLO;
  static uint16_t consumo = 0;
  static unsigned long tempo_ref = millis();
  static unsigned long tempo_TIMEOUT = millis();
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
        tempo_TIMEOUT = millis();
        modo_sys = PRINCIPAL;
      }
      break;
    case PRINCIPAL:
      modo_display = PRINCIPAL;
      consumo += quanto_bebeu();
      if (consumo >= 50){
        consumo_atual += consumo;
        consumo = 0;
        tempo_TIMEOUT = millis();
      }
      bateria = carga_bateria();
      if (botao_pressionado() == 1){
        modo_sys = IDADE;
      }
      if (millis()-tempo_TIMEOUT >= TEMPO_TIMEOUT){
        Timer_LED.reset();
        modo_sys = AVISO;
      }
      break;
    case AVISO:
      modo_display = AVISO;
      aviso_LED_buzzer(); 
      consumo += quanto_bebeu();
      if (B_OK == 1 or consumo >= 50){
        digitalWrite(LED, LOW);
        buzzer_logic(false);
        consumo_atual += consumo;
        consumo = 0;
        tempo_TIMEOUT = millis();
        modo_sys = PRINCIPAL;
      }
      break;
  }    
}

// Máquina de Estados Finita do Display

void controle_display(){
  switch(modo_display){
    case HELLO:
      exibirTela_hello();
      break;
    case IDADE:
      exibirTelaIdade();
      break;
    case PESO:
      exibirTelaPeso();
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

/* -FUNÇÕES QUE VÃO EXIBIR A TELA NO DISPLAY OLED
*/

//Gustavo
bool configuraDisplay(uint8_t textSize, uint16_t color = WHITE, int16_t xcoor = 0, int16_t ycoor = 0){
  //Essa função determina o tamanho do texto, cor e posição inicial
  if((ycoor < 0 || ycoor > SCREEN_HEIGHT) || (xcoor < 0 || xcoor > SCREEN_WIDTH))
    return false;
  display.setTextSize(textSize);
  display.setTextColor(color);
  display.setCursor(xcoor, ycoor);
  return true;
}

//Gustavo
void exibirTela_hello(){
  display.clearDisplay();
  configuraDisplay(10); //60x80
  display.print("Seja bem-vindo(a)");
}

//Gustavo
void exibirTelaIdade(){
  display.clearDisplay();
  configuraDisplay(3);
  display.print("Insira a sua Idade:");
  //converter de inteiro para string
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%d", idade);
  display.print(buffer);
}

//Gustavo
void exibirTelaPeso(){
  display.clearDisplay();
  configuraDisplay(3);
  display.print("Insira o seu peso:");
  //converter de inteiro para string
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%d", peso);
  display.print(buffer);
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
