#define CHARGE_PIN 2
#define SENSOR_PIN A4


void set_highspeed_adc();

unsigned long read_time(uint8_t sensor_pin, uint8_t charge_pin, uint8_t low, uint16_t high);
float capacitance_from_time();

uint16_t read_charge(uint8_t sensor_pin, uint8_t charge_pin);

float adjust(float input, float input_min, float input_max, float output_min, float output_max);

//346 leitura cheia
//instavel quando completamente vazio
//aproximadamente 150 quando com 5%

//calib 1105 - 630
//104 - 58


void setup(){

    Serial.begin(115200);

    pinMode(CHARGE_PIN, OUTPUT);
    digitalWrite(CHARGE_PIN, LOW);

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

void set_highspeed_adc(){
    ADCSRA &= ~(bit(ADPS0) | bit(ADPS1) | bit(ADPS2));
    ADCSRA |= bit(ADPS2);
}


unsigned long read_time(uint8_t sensor_pin, uint8_t charge_pin, uint8_t low, uint16_t high){
    
    digitalWrite(charge_pin, HIGH);
    unsigned long tempo = micros();

    while(analogRead(sensor_pin) < high);

    tempo = micros() - tempo;

    digitalWrite(charge_pin, LOW);
    while(analogRead(sensor_pin) > low);

    return tempo;
}


float capacitance_from_time(){
     unsigned long media_soma = 0;

    for(uint8_t i = 0 ; i < 10 ; i++){
        media_soma += read_time(SENSOR_PIN, CHARGE_PIN, 5, 885);
    }

    float media = (float)media_soma * 0.1;

    media -= 645.0;

    Serial.print("tempo: ");
    Serial.print(media);
    Serial.print("us");

    media = ((media * 0.000001) * 100000.0) * 0.5; 

    Serial.print(" - Capacitancia: ");
    Serial.print(media);
    Serial.println("pF");

    return media;
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