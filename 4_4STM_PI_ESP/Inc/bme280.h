#ifndef BME280_H
#define BME280_H

#include "main.h"
#include <stdint.h>

/* Тип знайденого чипа за chip ID */
typedef enum {
    BME_NONE   = 0,
    BME_BMP280 = 1,   /* ID 0x56/0x57/0x58, без вологості */
    BME_BME280 = 2    /* ID 0x60, з вологістю */
} bme_type_t;

typedef struct {
    int32_t  t_x100;    /* температура, °C * 100 */
    uint32_t rh_x100;   /* вологість, % * 100 (лише BME280) */
    uint32_t p_pa;      /* тиск, Па */
    int16_t  td_x100;   /* точка роси, °C * 100; INT16_MIN якщо немає вологості */
    uint8_t  has_rh;    /* 1 = BME280, 0 = BMP280 */
} bme_data_t;

/* Знаходить датчик (0x76, потім 0x77), читає калібрування, налаштовує режим normal */
HAL_StatusTypeDef BME280_Init(I2C_HandleTypeDef *hi2c);

/* Читає і компенсує значення. HAL_OK = дані коректні */
HAL_StatusTypeDef BME280_Read(bme_data_t *out);

bme_type_t BME280_Type(void);

#endif /* BME280_H */
