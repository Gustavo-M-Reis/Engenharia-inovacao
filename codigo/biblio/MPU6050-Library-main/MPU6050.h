#ifndef MPU6050_H
#define MPU6050_H


#include <Wire.h>
#include <Arduino.h>



class MPU6050{

    private:
        #define ADDR 0x68   //endereço do dispositivo
        #define PWR_1 0x6B  //controle de energia
        #define CONFIG 0X1A //configurações
        #define GYRO_CONFIG 0X1B    //configurações do giroscópio
        #define ACCEL_CONFIG 0X1C   //configuração do acelerometro
        #define GYRO 0  //identificador do Giroscópio
        #define ACCEL 1 //identificador do Acelerometro
       
        //constantes para conversãop de escala
        const float _gyro_conversion_const[4] = {
            131.0,
            65.5,
            32.8,
            16.4
        };


        const float _accel_conversion_const[4] = {
            16384,
            8192,
            4096,
            2048
        };

        uint8_t _gyro_range = 0;   //armazena a constante de conversão do giroscópio
        uint8_t _accel_range = 0;   //armazena a constante de conversão do acelerometro
        float _x_offset = 0.0;
        float _y_offset = 0.0;
        float _z_offset = 0.0;

        //envia dados 
        int8_t send_data(uint8_t addr, uint8_t data);
        //recebe dados
        int8_t request_data(uint8_t addr, uint8_t lenght, uint8_t *data);

        int8_t self_test(float &result, uint8_t addr, uint8_t sensor_type, unsigned int num_leituras);

        //calcula a média de n leituras do sensor especificado
        int8_t calc_mean(float &retorno, uint8_t sensor, unsigned int num_read);

       

    public:
        #define BANDWIDTH_5HZ  6
        #define BANDWIDTH_10HZ 5
        #define BANDWIDTH_20HZ 4
        #define BANDWIDTH_45HZ 3
        #define BANDWIDTH_95HZ 2
        #define BANDWIDTH_185HZ 1
        #define BANDWIDTH_260HZ 0

        #define GYRO_RANGE_250GS 0
        #define GYRO_RANGE_500GS 1
        #define GYRO_RANGE_1000GS 2
        #define GYRO_RANGE_2000GS 3

        #define ACCEL_RANGE_2G 0
        #define ACCEL_RANGE_4G 1
        #define ACCEL_RANGE_8G 2
        #define ACCEL_RANGE_16G 3

        #define GYRO_X 0X43
        #define GYRO_Y 0X45
        #define GYRO_Z 0X47

        #define ACCEL_X 0X3B
        #define ACCEL_Y 0X3D
        #define ACCEL_Z 0X3F

        #define TEMP 0x41

        //utilizado para self-test, Utiliza uma combinação binaria para selecionar os sensores
        #define ST_GX 0x01
        #define ST_GY 0x02
        #define ST_GZ 0x04
        #define ST_AX 0x08
        #define ST_AY 0x10
        #define ST_AZ 0x20


        //estrutura para armazenar dados do gyroscópio
        struct Deviation_data{float Gyro_x = 0, Gyro_y = 0, Gyro_z = 0;
                         float Accel_x = 0, Accel_y = 0, Accel_z = 0;}Deviation;
       

        
        //construtor
        MPU6050();

        //incia o sensor
        int8_t begin();

        //configuração do filtro
        int8_t lowPassFilter(uint8_t value);

        //configuração do range do giroscópio
        int8_t gyroscope_range(uint8_t value);

        //configuração do range do acelerometro
        int8_t accelerometer_range(uint8_t value);
        
        //coleta os dados de um sensor especifico
        int8_t get_sensor(uint8_t addr, int &value);


        float convert_gyro(float data);
        float convert_accel(float data);
        float convert_temp(int data);


        int8_t test_sensor(uint8_t sensor_select, unsigned int read_num);

        int8_t get_offset(float &retorno, uint8_t sensor, unsigned int num_read);

        


};



#endif
