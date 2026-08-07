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
#define SCREEN_HEIGHT 55 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define TEMPO_DEBOUNCE 100
#define TEMPO_ESPERA 5000
#define TEMPO_LED 500
#define TEMPO_TIMEOUT 60000 // Define o tempo de aviso como 60 segundos - apenas para teste

#define MIN_VALUE 130
#define MAX_VALUE 673


float _min_cap = 0.0;
float _max_cap = 0.0;

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
Timer Timer_stable;
Timer Tempo_ref;
Timer Tempo_TIMEOUT;
Timer Buzzer_timer;


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
uint8_t idade = 60;               // Idade inicializa com 60 anos - produto focado em idosos 
uint16_t peso = 70;                // Peso inicializa com 70 Kg
uint16_t consumo_est = 0;          // Consumo estimado - calculo com base em peso e idade
uint16_t consumo_atual = 0;        // Consumo atual - zera quando a garrafa é inicializada
uint8_t bateria = 0;              // Valor em porcentagem da bateria = bat_atual/bat_max
enum STATE modo_display = HELLO;       
uint32_t tempo_timeout = TEMPO_TIMEOUT;

// Declaração das Funções

// Funções - Igor
uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin);
float adjust(float input, float input_min, float input_max, float output_min, float output_max);
void buzzer_logic(uint8_t state);
bool is_stable();
uint16_t get_liq_level();
float value2cap(uint16_t value);
bool level_logic();


// Funções - Lucas
bool ler_botoes(Botao *btn);                             // Lê os botões e implementa debouncing                             
uint16_t carga_bateria();                                // Devolve a porcentagem da bateria
void controle_sys(bool B_UP, bool B_DW, bool B_OK);      // Implementa máquina de estados do sistema
void controle_display();                                 // Implementa máquina de estados do display
uint16_t consumo_estimado();                             // Calcula o consumo estimado baseado na idade e peso
void aviso_LED_buzzer();                                 // Controla LEDs e Buzzer
bool botao_pressionado();                                // Verifica se o botão foi pressionado por 5 segundos

bool configuraDisplay(uint8_t textSize, uint16_t color = WHITE, int16_t xcoor = 0, int16_t ycoor = 0);

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
    IMU.lowPassFilter(BANDWIDTH_5HZ);
    
    configuraDisplay(1);

    _min_cap = value2cap(MIN_VALUE);
    _max_cap = value2cap(MAX_VALUE);

    /*
    while(1){
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 2);

      uint16_t value = get_liq_level();

      display.print("nivel: ");
      display.println(value);

      if(is_stable()){
        display.print("Stable");
      }

      display.display();
    }
      */
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


uint16_t carga_bateria(){
  static float volts = ((float)analogRead(BAT_SENS) * 0.004887 - volts) * 0.01;
  volts += ((float)analogRead(BAT_SENS) * 0.004887 - volts) * 0.01;
  int16_t nivel = (int16_t)adjust(volts, 3.5, 4.2, 0.0, 100.0);
  nivel = max(min(100, nivel), 0);
  return (uint16_t)nivel;
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
  bool consumo;
  switch(modo_sys){
    case HELLO:
      modo_display = HELLO;
      if (Tempo_ref.get() >= TEMPO_ESPERA){
        Tempo_ref.reset();
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
        Tempo_ref.reset();
      }
      break;
    case SAVE:
      modo_display = SAVE;
      if (Tempo_ref.get() >= TEMPO_ESPERA){
        consumo_est = consumo_estimado();
        Tempo_TIMEOUT.reset();
        modo_sys = PRINCIPAL;
      }
      break;
    case PRINCIPAL:
      modo_display = PRINCIPAL;
      
      consumo = level_logic();

      if(consumo == true){
        Tempo_TIMEOUT.reset();
      }
      
      bateria = carga_bateria();
      if (botao_pressionado() == 1){
        modo_sys = IDADE;
      }
      if (Tempo_TIMEOUT.get() >= tempo_timeout){
        Timer_LED.reset();
        modo_sys = AVISO;
      }
      break;
    case AVISO:
      modo_display = AVISO;
      aviso_LED_buzzer(); 

      consumo = level_logic();
      
      if (B_OK == 1 or consumo == true){
        digitalWrite(LED, LOW);
        buzzer_logic(false);
        Tempo_TIMEOUT.reset();
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
      exibirTelaSave();
      break;
    case PRINCIPAL:
      exibirTelaPrincipal();
      break;
    case AVISO:
      exibirTelaAviso();
      break;
  }    
}

/* -FUNÇÕES QUE VÃO EXIBIR A TELA NO DISPLAY OLED
*/

//Gustavo
bool configuraDisplay(uint8_t textSize, uint16_t color, int16_t xcoor, int16_t ycoor){
  //Essa função determina o tamanho do texto, cor e posição inicial
  if((ycoor < 0 || ycoor > SCREEN_HEIGHT) || (xcoor < 0 || xcoor > SCREEN_WIDTH))
    return false;
  display.setTextSize(1);
  display.setTextColor(color);
  display.setCursor(xcoor, ycoor);
  return true;
}

//Gustavo
void exibirTela_hello(){
  display.clearDisplay();
  configuraDisplay(1); //60x80
  display.print("Seja bem-vindo(a)");
  display.display();
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
  display.display();
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
  display.display();
}

void exibirTelaSave(){
    display.clearDisplay();
    configuraDisplay(1);
    display.println("Dados salvos com");
    display.print("sucesso!");

    display.display();
}

void exibirTelaPrincipal(){
    display.clearDisplay();
    configuraDisplay(1);
    display.print("Bateria: ");
    display.print(carga_bateria());
    display.println("%");

    //display.setCursor(0,16);
    display.print(consumo_atual);
    display.print(" ml");
    display.print(" / ");
    display.print(consumo_estimado());
    display.print(" ml");

    display.display();
}

void exibirTelaAviso(){
    display.clearDisplay();
    configuraDisplay(1);
    display.println("Beba agua ou pres-");
    display.print("sione OK para adiar.");

    display.display();
}


// Essas são funções de sensoriamento escritas pelo Igor

float adjust(float input, float input_min, float input_max, float output_min, float output_max){
    return((output_max - output_min) / (input_max - input_min))  * (input - input_min) + output_min;
}

uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin){
    pinMode(sensor_pin, OUTPUT);
    
    digitalWrite(sensor_pin, LOW);
    digitalWrite(charge_pin, LOW);
    delay(1);

    pinMode(sensor_pin, INPUT);
    
    digitalWrite(charge_pin, HIGH);

    uint16_t result = analogRead(sensor_pin);
    digitalWrite(charge_pin, LOW);
    return result;
}

void buzzer_logic(uint8_t state){
  static byte current_state = 0;
  if(state){
    if(Buzzer_timer.get() >= 100){
      Buzzer_timer.reset();
      current_state = ~current_state;
      digitalWrite(BUZZER_PIN, current_state);
    }
  }
  else digitalWrite(BUZZER_PIN, LOW);
}

bool is_stable(){
  static float x_accel_filtered = 0.0, y_accel_filtered = 0.0, z_accel_filtered = 0.0;
  
  
  float alpha = 0.0;
  float theta = 0.0;


  float limiar = 700.0;
  float filtro_ativacao = 0.3;
 
  int x_accel = 0, y_accel = 0, z_accel = 0;      
  IMU.get_sensor(ACCEL_X, x_accel);
  IMU.get_sensor(ACCEL_Y, y_accel);
  IMU.get_sensor(ACCEL_Z, z_accel);


  float x = 0.0, y = 0.0, z = 0.0;
  x = (float)abs(x_accel);
  y = (float)abs(y_accel);
  z = (float)abs(z_accel);

  x_accel_filtered += (x - x_accel_filtered) * filtro_ativacao;
  y_accel_filtered += (y - y_accel_filtered) * filtro_ativacao;
  z_accel_filtered += (z - z_accel_filtered) * filtro_ativacao;

  alpha = RAD_TO_DEG * atan2(x_accel_filtered, z_accel_filtered);
  theta = RAD_TO_DEG * atan2(y_accel_filtered, z_accel_filtered);


  if(x > (x_accel_filtered + limiar)){
    Timer_stable.reset();
    return false;
  }


  if(y > (y_accel_filtered + limiar)){
    Timer_stable.reset();
    return false;
  }


  if(z > (z_accel_filtered + limiar)){
    Timer_stable.reset();
    return false;
  }



  if(alpha > 5.0 || theta > 5.0){
    Timer_stable.reset();
    return false;
  }

  if(Timer_stable.get() > 2000) return true;

  return false;
    
}

//faz a leitura do nivel de fluido na garrafa
uint16_t get_liq_level(){

  /*
os valores devem sar calibrados de acordo com a quantidade
calibração com os pontos abaixo:
0	123
24	205
50	268
88	337
138	407
263	528
360	590
464	642
545	673
valores aferidos com uma balança
*/

uint32_t media = 0;
for(uint8_t i = 0 ; i < 20 ; i++){
  media += read_charge(SENSOR_PIN, CHARGE_PIN);
}

float capacitancia = value2cap((float)media / 20.0);

int16_t nivel = (int16_t)adjust(capacitancia, _min_cap, _max_cap, 0.0, 545.0);

nivel = min(max(nivel, 0), 550);

display.print("raw: ");
display.println((uint16_t)media / 20);

//Serial.print("raw value: ");
//Serial.print(raw_value);
//Serial.print(" - level: ");
//Serial.println(out);

return((uint16_t)nivel);
}


float value2cap(uint16_t value){

  float volts = 0.00488758553 * (float)value;
  float capacitancia = (volts * 47.0) / (4.5 - volts);

  return capacitancia;
}


bool level_logic(){
  // gerencia a lógica de consumo identificando pontos chave como, movimentação, e mudanças no nivel, para cima e para baixo
  static bool last_stable_state = false;
  static int16_t nivel = get_liq_level(), last_nivel = 0;
  //armazena o nivel da garrafa quando cheia, evita o acumulo de erro na medição
  static int16_t full_nivel = nivel;
  static int16_t consumo_acumulado = 0;

  bool stable = is_stable();

  //a leitura de nivel deve ser feita somente após a sequencia is_stable() -> false -> true, e somente uma vez, para evitar interferencia devido o toque
  if(last_stable_state == false && stable == true){
    last_stable_state = true;
    nivel = get_liq_level();

    int16_t consumo = nivel - last_nivel;

    //determina se a garrafa foi enchida
    if(consumo >= 50){
      consumo_acumulado = consumo_atual;
      last_nivel = nivel;
      full_nivel = nivel;
      return false;
    }

    //atualiza consumo caso a mudança de nivel seja superior ao valor minimo
    if(consumo <= 10){
      consumo_atual = (full_nivel - nivel) + consumo_acumulado;
      last_nivel = nivel;
      tempo_timeout = (uint32_t)(((float)abs(consumo) / 800.0) * 3600.0) * 1000;
      return true;
    }
  }
  else if(stable == false){
    last_stable_state = false;
  }
  return false;
}