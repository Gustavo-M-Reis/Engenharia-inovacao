
#include "MPU6050.h"
#include <Wire.h>
#include <Arduino.h>




//Inicialização da biblioteca Wire
MPU6050::MPU6050(){
    Wire.begin();
}


//envia dados para o dispositivo
// (endereço_do_registrador, dado)
//retorna 0 se tudo ocorrer bem, !=0 em caso de erro
//1: data too long to fit in transmit buffer.
//2: received NACK on transmit of address.
//3: received NACK on transmit of data.
//4: other error.
//5: timeout
int8_t MPU6050::send_data(uint8_t addr, uint8_t data){
    Wire.beginTransmission(ADDR);   //inicia transmição
    Wire.write(addr);               //envia registrador
    Wire.write(data);               //envia dado
    return (int8_t)Wire.endTransmission(true);     //finaliza transmição
}


//solicita dados do sensor
//(endereço_do_registrador, numero_de_bytes, ponteiro de retorno)
int8_t MPU6050::request_data(uint8_t addr, uint8_t lenght, uint8_t *data){
    
    //seta o registrador a solocitar dados e lida com erros
    Wire.beginTransmission(ADDR);
    Wire.write(addr);
    uint8_t status_return = Wire.endTransmission(false);
    if(status_return)return (int8_t)status_return;  //em caso de erro, retorna o código

    //solicita os dados
    status_return = Wire.requestFrom(ADDR, (int)lenght, true);
    if(status_return != lenght)return -1;   //lida com possiveis erros
    
    //move os dados para a variavel de saída 
    for(uint8_t i = 0 ; i < lenght ; i++){
        *(data + i) = Wire.read();
    }

    return 0;
}



//Inicialização do sensor
int8_t MPU6050::begin(){
    Wire.setWireTimeout(3000, true);
    return send_data(PWR_1, 0x00);     //incia a alimentação do sensor
}


//configura o filtro passa-baixa
int8_t MPU6050::lowPassFilter(uint8_t value){
    if(value > 6) return -1;    //checa range de entrada
    return (CONFIG, value);    
}


int8_t MPU6050::gyroscope_range(uint8_t value){
    if(value > 3) return -1;    //checa range de entrada

    //envia os dados e lida com erros
    int8_t status_return = send_data(GYRO_CONFIG, value<<3);
    if(status_return) return status_return;

    //armazenando o range escolhido
    _gyro_range = value;

    return 0;
}

int8_t MPU6050::accelerometer_range(uint8_t value){
    if(value > 3) return -1;    //checa range de entrada

    //envia os dados e lida com erros
    int8_t status_return = send_data(ACCEL_CONFIG, value<<3);
    if(status_return) return status_return;

    //armazenando o range escolhido
    _accel_range = value;

    return 0;
}


//coleta o valor do sensor em addr e devolve em value
int8_t MPU6050::get_sensor(uint8_t addr, int &value){
    uint8_t data[2];
    int8_t status_return = request_data(addr, 2, data);
    if(status_return) return status_return;

    value = (int)((data[0]<<8) | data[1]);

    return 0;
}



int8_t MPU6050::self_test(float &result, uint8_t addr, uint8_t sensor_type, unsigned int num_leituras){
    //registradores de controle
    const uint8_t config_reg[2] = {0x1b, 0x1c};
    //endereço dos sensores
    const uint8_t sensor_addr[2][3] = {GYRO_X, GYRO_Y, GYRO_Z, ACCEL_X, ACCEL_Y, ACCEL_Z};

    const uint8_t test_range[2] = {0, 0X10}; 

    const uint8_t st_info[3] = {0X0D, 0X0E, 0X0F};

    float *offset[3] = {&_x_offset, &_y_offset, &_z_offset};

    //copia range atual, e a configuração do filtro
    uint8_t reg_data = 0;
    uint8_t filter_data = 0;
    int8_t return_status = 0;

    return_status += request_data(config_reg[sensor_type], 1, &reg_data);
    return_status += request_data(0X1A, 1, &filter_data);

    if(return_status) return -1;

    //seta filtro para zero
    lowPassFilter(BANDWIDTH_260HZ);
    
    //filtra bits 3 e 4 
    reg_data &= 0x18;

    //desliga self test e seta range para o range de teste 10
    return_status += send_data(config_reg[sensor_type], test_range[sensor_type]);
    delay(2);

    //coleta dados do sensor com self test deligado
    float pre_value = 0.0;

    if(calc_mean(pre_value, sensor_addr[sensor_type][addr], num_leituras)) return -1;

    Serial.print("pre: ");
    Serial.print(pre_value);



    //liga self test
    return_status += send_data(config_reg[sensor_type],(0x01 << (7 - addr)));
    delay(2);

    if(return_status) return -1;

    //coleta dados do sensor com self test ligado
    float value = 0.0;
    if(calc_mean(value, sensor_addr[sensor_type][addr], num_leituras)) return -1;

    Serial.print(" - pos value: ");
    Serial.print(value);

    //desliga self test e reconfigura registradores
    return_status += send_data(config_reg[sensor_type], reg_data);
    return_status += send_data(0X1A, filter_data);
    delay(2);

    if(return_status) return -1;

    //coleta constante do sensor sendo testado
    //coleta primeira parte do valor (no caso de um acelerometro)
    uint8_t sensor_const = 0;
    return_status += request_data(st_info[addr], 1, &sensor_const);

    //verifica se se trata de um acelerometro
    if(sensor_type){
        //recorta primeiro valor e desloca
        sensor_const = (sensor_const >> 3) & 0x1C;

        //coleta segundo valor
        uint8_t second_value = 0;
        return_status += request_data(0x10, 1, &second_value);
        
        //isola valor e posiciona e concatena
        sensor_const |= ((second_value >> (4 - (2 * addr))) & 0x03);
    }
    //caso seja um giroscopio acerta os dados
    else sensor_const &= 0x1F;

    if(return_status) return -1;

    Serial.print(" - sensor const: ");
    Serial.print(sensor_const);

    //computa o Factory Trim
    float factory_trim = 0.0;
    if(sensor_const == 0)factory_trim = 0.0;
    else{
        //caso giroscopio eixo y
        if(!sensor_type && addr == 1){
            factory_trim = (-25.0*131.0*pow(1.046, (float)sensor_const));
            Serial.print(" - aqui1 ");
        }
        //caso giroscopio eixo x ou z
        else if(!sensor_type){
            factory_trim = (25.0*131.0*pow(1.046, (float)sensor_const));
            Serial.print(" - aqui2 ");
        }
        //caso acelerometro
        else{
            factory_trim = 4096.0 * 0.34 * (pow((0.92 / 0.94), ((float)sensor_const - 1.0) / (pow(2.0,5.0) - 2.0)));
            Serial.print(" - aqui3 ");
        }
    }

    Serial.print(" - factory trim: ");
    Serial.print(factory_trim);



    //computa o resultado
    result = (((value - pre_value) - factory_trim)/factory_trim);

    Serial.print(" - resultado: ");
    Serial.println(result);

    if(return_status) return -1;

    return 0;
}




//Realiza o teste do sensor especificado e retorna o erro escalar
int8_t MPU6050::test_sensor(uint8_t sensor_select, unsigned int read_num){
    //checa se o sensor a ser checado se trata de um giroscopio ou um acelerometro

    //no caso de um Gisroscopio
    //desliga auto teste, copia range atual, seta para rage de auto teste, coleta dados, liga auto teste no eixo selecionado, faz coleta de dados
    //desliga auto teste, retorna configurações originais, calcula Factory trim
    
    //Identificadores dos sensores
    //ST_GX 0x01
    //ST_GY 0x02
    //ST_GZ 0x04
    //ST_AX 0x08
    //ST_AY 0x10
    //ST_AZ 0x20
    

    if(sensor_select & ST_GX){
        self_test(Deviation.Gyro_x, 0, GYRO, read_num);
        
    }
    if(sensor_select & ST_GY){
        self_test(Deviation.Gyro_y, 1, GYRO, read_num);
        
    }
    if(sensor_select & ST_GZ){
        self_test(Deviation.Gyro_z, 2, GYRO, read_num);
        
    }
    if(sensor_select & ST_AX){
        self_test(Deviation.Accel_x, 0, ACCEL, read_num);
        
    }
    if(sensor_select & ST_AY){
        self_test(Deviation.Accel_y, 1, ACCEL, read_num);
       
    }
    if(sensor_select & ST_AZ){
        self_test(Deviation.Accel_z, 2, ACCEL, read_num);
       
    }

    return 0;
}


//calcula o offset do sensor solicitado
int8_t MPU6050::get_offset(float &retorno, uint8_t sensor, unsigned int num_read){
    if(calc_mean(retorno, sensor, num_read)) return -1;

    return 0;
}



float MPU6050::convert_gyro(float data){
    return data/_gyro_conversion_const[_gyro_range];
}


float MPU6050::convert_accel(float data){
    return data/_accel_conversion_const[_accel_range];
}

float MPU6050::convert_temp(int data){
    return ((float)data / 340.0) + 36.53;
}


//calcula a média de n leituras
int8_t MPU6050::calc_mean(float &retorno, uint8_t sensor, unsigned int num_read){

    int buff;
    retorno = 0;
    uint8_t return_status = 0;
    
    for(float i = 1 ; i <= num_read ; i++){
        if(get_sensor(sensor, buff)) return -1;
        retorno += ((float)buff - retorno)/i;
        delay(1);
    }


    return 0;
}
