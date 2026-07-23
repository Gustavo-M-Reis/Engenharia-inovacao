#include <MPU6050.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>


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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);



uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin);

float adjust(float input, float input_min, float input_max, float output_min, float output_max);
void buzzer_logic(byte state);




//346 leitura cheia
//instavel quando completamente vazio
//aproximadamente 150 quando com 5%

//calib 1105 - 630
//104 - 58

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

void move_detect();

MPU6050 IMU;



void setup(){

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
    while(1){
        display.clearDisplay();

        display.setTextSize(1);      // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE); // Draw white text
        display.setCursor(0, 0);     // Start at top-left corner
        display.cp437(true);         // Use full 256 char 'Code Page 437' font

        display.print("CIMA: ");
        display.println(!digitalRead(BOT_CIMA));
        display.print("BAIXO: ");
        display.println(!digitalRead(BOT_BAIXO));
        display.print("CONFIRMA: ");
        display.println(!digitalRead(BOT_CONFIRMA));
        display.print("Tensao BAT: ");
        display.println((float)analogRead(BAT_SENS) * 0.004887);
    

        display.display();
       

        if(Timer_LED.get() < 500){
            digitalWrite(LED, HIGH);
        }
        else if(Timer_LED.get() >= 500 && Timer_LED.get() < 1000){
            digitalWrite(LED, LOW);
        }
        else{
            Timer_LED.reset();
        }

        
        if(Timer_IMU.get() > 5){
            IMU.get_sensor(ACCEL_Z, z_accel);
            Timer_IMU.reset();
        }
        

        if(z_accel < 0){
            buzzer_logic(true);
        }
        else buzzer_logic(false);

    }
    
}


void loop(){

    unsigned long soma_media = 0;
    for(uint8_t i = 0 ; i < 40 ; i++){
        soma_media += read_charge(SENSOR_PIN, CHARGE_PIN);
    }

    float media = (float)soma_media * 0.025;

    Serial.print("leitura: ");
    Serial.print(media);

    float volts = media * (5.0 / 1023.0);
    float cap = ((volts * 47) / (5.0 - volts));

    /*
    630 - 58
    1105 - 104
    */

    cap = adjust(cap, 58.0, 630.0, 104.0, 1105.0);

    Serial.print(" - capacitancia: ");
    Serial.print(cap);

    media = adjust(media, 67.0, 346.0, 0, 100);

    Serial.print(" - nivel: ");
    Serial.println(media);


}

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
    float x_accel_filtered = 0.0, y_accel_filtered = 0.0, z_accel_filtered = 0.0;
    int x_accel = 0, y_accel = 0, z_accel = 0;

    float allarm_filter = 0.0;

    Timer read_timer;

    float x = 0.0, y = 0.0, z = 0.0;

    float limiar = 300.0;

    while(1){
        if(read_timer.get() > 5){
            IMU.get_sensor(ACCEL_X, x_accel);
            IMU.get_sensor(ACCEL_Y, y_accel);
            IMU.get_sensor(ACCEL_Z, z_accel);
            read_timer.reset();

            x = (float)abs(x_accel);
            y = (float)abs(y_accel);
            z = (float)abs(z_accel);

            x_accel_filtered += (x - x_accel_filtered) * 0.3;
            y_accel_filtered += (y - y_accel_filtered) * 0.3;
            z_accel_filtered += (z - z_accel_filtered) * 0.3;
        }

        if(x > (x_accel_filtered + limiar)){
            allarm_filter += ((x - x_accel_filtered + limiar)) * 0.1;
        }
        else allarm_filter += (-allarm_filter) * 0.01;

        if(y > (y_accel_filtered + limiar)){
            allarm_filter += ((y - y_accel_filtered + limiar)) * 0.1;
        }
        else allarm_filter += (-allarm_filter) * 0.01;

        if(z > (z_accel_filtered + limiar)){
            allarm_filter += ((z - z_accel_filtered + limiar)) * 0.1;
        }
        else allarm_filter += (-allarm_filter) * 0.01;

        if(allarm_filter > 1000) buzzer_logic(true);
        else buzzer_logic(false);

    }
}

