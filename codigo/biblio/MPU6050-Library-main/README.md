
# MPU6050 Arduino Library

A lightweight Arduino library to interface with the MPU6050 6-axis accelerometer and gyroscope via I2C, designed for ease of configuration and data acquisition.

---

## Features

- Initialize MPU6050 with one command
- Configure low-pass filter bandwidth
- Set gyroscope and accelerometer sensitivity
- Perform self-tests and get offset data
- Read raw sensor data
- Convert raw data to physical units (°/s, g, °C)

---

## Installation

1. Clone or download this repository.
2. Copy the `MPU6050.cpp` and `MPU6050.h` files into your Arduino project directory.
3. Include the header in your sketch:

```cpp
#include "MPU6050.h"
```

---

## Example

This example initializes the sensor, configures:
- Low-pass filter to 45Hz
- Gyroscope range to ±250°/s
- Accelerometer range to ±4g

Then reads raw and converted sensor data:

```cpp
#include <Wire.h>
#include "MPU6050.h"

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (mpu.begin() != 0) {
    Serial.println("Failed to initialize MPU6050");
    while (1);
  }

  mpu.lowPassFilter(BANDWIDTH_45HZ);
  mpu.gyroscope_range(GYRO_RANGE_250GS);
  mpu.accelerometer_range(ACCEL_RANGE_4G);

  Serial.println("MPU6050 ready.");
}

void loop() {
  int raw_gx, raw_gy, raw_gz;
  int raw_ax, raw_ay, raw_az;
  int raw_temp;

  mpu.get_sensor(GYRO_X, raw_gx);
  mpu.get_sensor(GYRO_Y, raw_gy);
  mpu.get_sensor(GYRO_Z, raw_gz);

  mpu.get_sensor(ACCEL_X, raw_ax);
  mpu.get_sensor(ACCEL_Y, raw_ay);
  mpu.get_sensor(ACCEL_Z, raw_az);

  mpu.get_sensor(TEMP, raw_temp);

  Serial.print("Gyro (°/s): ");
  Serial.print(mpu.convert_gyro(raw_gx)); Serial.print(", ");
  Serial.print(mpu.convert_gyro(raw_gy)); Serial.print(", ");
  Serial.println(mpu.convert_gyro(raw_gz));

  Serial.print("Accel (g): ");
  Serial.print(mpu.convert_accel(raw_ax)); Serial.print(", ");
  Serial.print(mpu.convert_accel(raw_ay)); Serial.print(", ");
  Serial.println(mpu.convert_accel(raw_az));

  Serial.print("Temp (°C): ");
  Serial.println(mpu.convert_temp(raw_temp));

  delay(1000);
}
```

---

## API Reference

### Constructor

```cpp
MPU6050 mpu;
```
### Return Values

All public methods that return `int8_t` will return `0` on success.

If any function returns a value different from zero, this indicates a communication error or a problem with the sensor. Typical I2C error codes can include:

-   `1`: Data too long for transmit buffer
    
-   `2`: Received NACK on transmit of address
    
-   `3`: Received NACK on transmit of data
    
-   `4`: Other error
    
-   `5`: Timeout
    
-   `-1`: General error in data request or out-of-range parameter
### Sensor Control


- `int8_t begin()`: Initializes the sensor and powers it on.
- `int8_t lowPassFilter(uint8_t value)`: Configures the digital low-pass filter.
- `int8_t gyroscope_range(uint8_t value)`: Sets gyroscope range (0=250, 1=500, 2=1000, 3=2000 °/s).
- `int8_t accelerometer_range(uint8_t value)`: Sets accelerometer range (0=2g, 1=4g, 2=8g, 3=16g).

### Reading Data

- `int8_t get_sensor(uint8_t addr, int &value)`: Reads 16-bit raw data from a specific sensor register.
- `float convert_gyro(float data)`: Converts raw gyro value to °/s.
- `float convert_accel(float data)`: Converts raw accelerometer value to g.
- `float convert_temp(int data)`: Converts raw temperature to degrees Celsius.

### Advanced

- `int8_t get_offset(float &result, uint8_t sensor_addr, unsigned int samples)`: Calculates sensor offset by taking the mean n samples.

---

## Constants

Low-pass filter options:
- `BANDWIDTH_5HZ`, `BANDWIDTH_10HZ`, `BANDWIDTH_20HZ`, `BANDWIDTH_45HZ`,
-  `BANDWIDTH_95HZ`, `BANDWIDTH_185HZ`, `BANDWIDTH_260HZ`

Gyroscope range:
- `GYRO_RANGE_250GS`, `GYRO_RANGE_500GS`, `GYRO_RANGE_1000GS`, `GYRO_RANGE_2000GS`

Accelerometer range:
- `ACCEL_RANGE_2G`, `ACCEL_RANGE_4G`, `ACCEL_RANGE_8G`, `ACCEL_RANGE_16G`

Sensor register addresses:
- `GYRO_X`, `GYRO_Y`, `GYRO_Z`, `ACCEL_X`, `ACCEL_Y`, `ACCEL_Z`, `TEMP`

---

## License

MIT License

```
MIT License

Copyright (c) 2025 Igor Tironi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

...

```

---

## Credits

- **Development**: Igor Tironi  
- **Documentation**: Generated with help from ChatGPT

