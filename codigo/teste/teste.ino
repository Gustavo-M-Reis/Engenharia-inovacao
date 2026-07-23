#include <MPU6050.h>
#include <Wire.h>


#define CHARGE_PIN 10
#define SENSOR_PIN A0
#define BUZZER_PIN 6


uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin);

float adjust(float input, float input_min, float input_max, float output_min, float output_max);
void demo();
void buzzer_logic(byte state);


//346 leitura cheia
//instavel quando completamente vazio
//aproximadamente 150 quando com 5%

//calib 1105 - 630
//104 - 58

MPU6050 IMU;


void setup(){

    Serial.begin(115200);

    pinMode(CHARGE_PIN, OUTPUT);
    digitalWrite(CHARGE_PIN, LOW);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    if(IMU.begin()){
        Serial.println("erro ao iniciar a MPU");
        while(1);
    }


    demo();
    
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


void demo(){

    unsigned long timer = 0;
    int z_accel = 0;
    while(1){
        
        if(millis() - timer > 5){
            IMU.get_sensor(ACCEL_Z, z_accel);
            timer = millis();
        }
        

        if(z_accel < 0){
            buzzer_logic(true);
        }
        else buzzer_logic(false);
    }
  

}