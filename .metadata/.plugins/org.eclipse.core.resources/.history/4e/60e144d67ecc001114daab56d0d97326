#include "bme280.h"
#include <math.h>
#include <string.h>

#define BME_ADDR_A      (0x76 << 1)
#define BME_ADDR_B      (0x77 << 1)

#define REG_ID          0xD0
#define REG_RESET       0xE0
#define REG_CTRL_HUM    0xF2
#define REG_STATUS      0xF3
#define REG_CTRL_MEAS   0xF4
#define REG_CONFIG      0xF5
#define REG_DATA        0xF7
#define REG_CALIB_00    0x88
#define REG_CALIB_26    0xE1

#define I2C_TIMEOUT_MS  50

static I2C_HandleTypeDef *s_i2c;
static uint16_t s_addr;
static bme_type_t s_type = BME_NONE;

/* Калібрувальні коефіцієнти */
static uint16_t dig_T1; static int16_t dig_T2, dig_T3;
static uint16_t dig_P1; static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t dig_H1, dig_H3; static int16_t dig_H2, dig_H4, dig_H5; static int8_t dig_H6;
static int32_t t_fine;

static HAL_StatusTypeDef rd(uint8_t reg, uint8_t *buf, uint16_t n)
{
    return HAL_I2C_Mem_Read(s_i2c, s_addr, reg, I2C_MEMADD_SIZE_8BIT, buf, n, I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef wr(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(s_i2c, s_addr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, I2C_TIMEOUT_MS);
}

static uint16_t u16le(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  s16le(const uint8_t *p) { return (int16_t)u16le(p); }

bme_type_t BME280_Type(void) { return s_type; }

HAL_StatusTypeDef BME280_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t id = 0;
    const uint16_t addrs[2] = { BME_ADDR_A, BME_ADDR_B };
    int found = 0;

    s_i2c = hi2c;
    s_type = BME_NONE;

    for (int i = 0; i < 2 && !found; i++) {
        s_addr = addrs[i];
        if (rd(REG_ID, &id, 1) == HAL_OK) {
            if (id == 0x60) { s_type = BME_BME280; found = 1; }
            else if (id == 0x56 || id == 0x57 || id == 0x58) { s_type = BME_BMP280; found = 1; }
        }
    }
    if (!found) return HAL_ERROR;

    /* Програмний скид і очікування завершення завантаження калібрування */
    if (wr(REG_RESET, 0xB6) != HAL_OK) return HAL_ERROR;
    HAL_Delay(10);
    for (int i = 0; i < 20; i++) {
        uint8_t st = 0;
        if (rd(REG_STATUS, &st, 1) == HAL_OK && (st & 0x01) == 0) break;
        HAL_Delay(5);
    }

    /* Калібрування температури і тиску: 0x88..0xA1 (26 байт) */
    uint8_t c[26];
    if (rd(REG_CALIB_00, c, 26) != HAL_OK) return HAL_ERROR;
    dig_T1 = u16le(&c[0]);  dig_T2 = s16le(&c[2]);  dig_T3 = s16le(&c[4]);
    dig_P1 = u16le(&c[6]);  dig_P2 = s16le(&c[8]);  dig_P3 = s16le(&c[10]);
    dig_P4 = s16le(&c[12]); dig_P5 = s16le(&c[14]); dig_P6 = s16le(&c[16]);
    dig_P7 = s16le(&c[18]); dig_P8 = s16le(&c[20]); dig_P9 = s16le(&c[22]);
    dig_H1 = c[25];

    if (s_type == BME_BME280) {
        uint8_t h[7];
        if (rd(REG_CALIB_26, h, 7) != HAL_OK) return HAL_ERROR;
        dig_H2 = s16le(&h[0]);
        dig_H3 = h[2];
        dig_H4 = (int16_t)(((int8_t)h[3] * 16) | (h[4] & 0x0F));
        dig_H5 = (int16_t)(((int8_t)h[5] * 16) | (h[4] >> 4));
        dig_H6 = (int8_t)h[6];
        if (wr(REG_CTRL_HUM, 0x01) != HAL_OK) return HAL_ERROR;   /* osrs_h x1 */
    }

    /* standby 1000 мс, фільтр вимкнено */
    if (wr(REG_CONFIG, 0xA0) != HAL_OK) return HAL_ERROR;
    /* osrs_t x1, osrs_p x1, normal mode (запис ctrl_meas активує ctrl_hum) */
    if (wr(REG_CTRL_MEAS, 0x27) != HAL_OK) return HAL_ERROR;

    HAL_Delay(20);
    return HAL_OK;
}

/* Температура, 0.01 °C. Оновлює t_fine */
static int32_t comp_T(int32_t adc_T)
{
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t v2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12)
                  * ((int32_t)dig_T3)) >> 14;
    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8;
}

/* Тиск, формат Q24.8 Па */
static uint32_t comp_P(int32_t adc_P)
{
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)dig_P6;
    v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
    v2 = v2 + (((int64_t)dig_P4) << 35);
    v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
    v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)dig_P1) >> 33;
    if (v1 == 0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    v2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + v1 + v2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)p;
}

/* Вологість, формат Q22.10 %RH */
static uint32_t comp_H(int32_t adc_H)
{
    int32_t v = t_fine - 76800;
    v = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v)) + 16384) >> 15)
         * (((((((v * ((int32_t)dig_H6)) >> 10) * (((v * ((int32_t)dig_H3)) >> 11) + 32768)) >> 10)
              + 2097152) * ((int32_t)dig_H2) + 8192) >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (uint32_t)(v >> 12);
}

/* Точка роси за формулою Магнуса (над водою) */
static int16_t dewpoint_x100(int32_t t_x100, uint32_t rh_x100)
{
    float t  = t_x100 / 100.0f;
    float rh = rh_x100 / 10000.0f;
    if (rh < 0.01f) rh = 0.01f;
    if (rh > 1.0f)  rh = 1.0f;
    const float b = 17.62f, c = 243.12f;
    float g  = logf(rh) + (b * t) / (c + t);
    float td = c * g / (b - g);
    return (int16_t)lroundf(td * 100.0f);
}

HAL_StatusTypeDef BME280_Read(bme_data_t *out)
{
    if (s_type == BME_NONE || out == NULL) return HAL_ERROR;

    uint8_t d[8] = {0};
    uint16_t n = (s_type == BME_BME280) ? 8 : 6;
    if (rd(REG_DATA, d, n) != HAL_OK) return HAL_ERROR;

    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    if (adc_T == 0x80000 || adc_P == 0x80000) return HAL_ERROR;   /* вимір пропущено */

    out->t_x100 = comp_T(adc_T);
    out->p_pa   = comp_P(adc_P) / 256;

    if (s_type == BME_BME280) {
        int32_t adc_H = ((int32_t)d[6] << 8) | d[7];
        uint32_t h = comp_H(adc_H);                    /* Q22.10 */
        out->rh_x100 = (h * 100u) / 1024u;
        out->has_rh  = 1;
        out->td_x100 = dewpoint_x100(out->t_x100, out->rh_x100);
    } else {
        out->rh_x100 = 0xFFFF;
        out->has_rh  = 0;
        out->td_x100 = INT16_MIN;
    }

    /* Перевірка на розумність (-40..+85 °C, 300..1100 hPa) */
    if (out->t_x100 < -4000 || out->t_x100 > 8500) return HAL_ERROR;
    if (out->p_pa < 30000 || out->p_pa > 110000)   return HAL_ERROR;
    return HAL_OK;
}
