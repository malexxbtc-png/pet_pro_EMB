#include "app.h"
#include "main.h"
#include "bme280.h"
#include "i2c_slave.h"
#include <string.h>

/* Визначені в main.c (CubeMX). У main.c з MX_I2C2_Init треба прибрати слово static */
extern I2C_HandleTypeDef hi2c2;
void MX_I2C2_Init(void);

#if defined(HAL_IWDG_MODULE_ENABLED)
extern IWDG_HandleTypeDef hiwdg;
#endif

#define SENSOR_PERIOD_MS   1000
#define SENSOR_FAIL_LIMIT  3

static uint8_t  s_block[10];
static uint8_t  s_status;
static uint8_t  s_heartbeat;
static uint8_t  s_sensor_ok;
static uint8_t  s_fail_cnt;
static uint32_t s_t_sensor;

/* Відновлення шини I2C2: 9 імпульсів SCL, STOP, повторна ініціалізація */
static void i2c2_recover(void)
{
    HAL_I2C_DeInit(&hi2c2);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_10 | GPIO_PIN_3;       /* SCL, SDA */
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(1);
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_Delay(1);
    }
    /* Умова STOP */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(1);

    MX_I2C2_Init();
}

static void pack_block(const bme_data_t *d)
{
    int16_t  t  = (int16_t)d->t_x100;
    uint16_t rh = (uint16_t)d->rh_x100;
    uint32_t p  = d->p_pa;
    int16_t  td = d->td_x100;

    s_block[0] = (uint8_t)(t & 0xFF);          s_block[1] = (uint8_t)((uint16_t)t >> 8);
    s_block[2] = (uint8_t)(rh & 0xFF);         s_block[3] = (uint8_t)(rh >> 8);
    s_block[4] = (uint8_t)(p & 0xFF);          s_block[5] = (uint8_t)((p >> 8) & 0xFF);
    s_block[6] = (uint8_t)((p >> 16) & 0xFF);  s_block[7] = (uint8_t)((p >> 24) & 0xFF);
    s_block[8] = (uint8_t)(td & 0xFF);         s_block[9] = (uint8_t)((uint16_t)td >> 8);
}

static void publish(void)
{

    I2CS_SetRegs(s_status, s_heartbeat, s_block);
}

/* Коротке блимання на старті: count спалахів по on_ms (LED активний 0) */
static void startup_blink(uint32_t on_ms, int count)
{
    for (int i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        HAL_Delay(on_ms);
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        HAL_Delay(on_ms);
    }
}

void App_Init(void)
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   /* LED вимкнено (активний 0) */

    memset(s_block, 0, sizeof(s_block));
    s_block[2] = 0xFF; s_block[3] = 0xFF;                       /* RH = N/A */
    s_block[8] = 0x00; s_block[9] = 0x80;                       /* Td = INT16_MIN (N/A) */

    s_sensor_ok = (BME280_Init(&hi2c2) == HAL_OK);
    s_status = s_sensor_ok ? STATUS_SENSOR_OK : 0;
    if (s_sensor_ok && BME280_Type() == BME_BME280) s_status |= STATUS_HAS_RH;


    I2CS_Init();
    publish();


    startup_blink(150, 3);

    s_t_sensor = HAL_GetTick();
}

void App_Loop(void)
{
#if defined(HAL_IWDG_MODULE_ENABLED)
    HAL_IWDG_Refresh(&hiwdg);
#endif
    I2CS_Poll();

    uint32_t now = HAL_GetTick();

    /* Раз на 1 с: читання датчика і оновлення регістрів */
    if ((now - s_t_sensor) >= SENSOR_PERIOD_MS) {
        s_t_sensor = now;
        s_heartbeat++;

        if (!s_sensor_ok) {
            if (BME280_Init(&hi2c2) == HAL_OK) {
                s_sensor_ok = 1;
                s_fail_cnt = 0;
            } else {
                i2c2_recover();
            }
        } else {
            bme_data_t d;
            if (BME280_Read(&d) == HAL_OK) {
                s_fail_cnt = 0;
                pack_block(&d);
            } else if (++s_fail_cnt >= SENSOR_FAIL_LIMIT) {
                s_sensor_ok = 0;
                i2c2_recover();
            }
        }

        s_status = s_sensor_ok ? STATUS_SENSOR_OK : 0;
        if (s_sensor_ok && BME280_Type() == BME_BME280) s_status |= STATUS_HAS_RH;
        publish();
    }

    /* Індикація від поточного часу (без накопичення):
       норма = 1 Гц (500 мс горить / 500 мс ні), немає BME/BMP280 = 4 Гц.
       Якщо діод не блимає, головний цикл завис або плата скидається. */
    uint8_t on = ((now % 1000) < 500);
    if (!s_sensor_ok) on = ((now % 250) < 125);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
