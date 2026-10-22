/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (Приклад 8 + PIR AM312 управління сном)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RTC_ADDR   (0x68 << 1)   // 0xD0 для DS1307[cite: 3]
#define OLED_ADDR  (0x3C << 1)   // 0x78 для SSD1306[cite: 3]
#define INACTIVITY_TIMEOUT_MS 300000 // 5 хвилин неактивності до вимикання дисплея
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
uint8_t hour = 0, min = 0, sec = 0;
uint32_t lastMotionTime = 0;
uint8_t displayState = 1; // 1 - Дисплей увімкнений, 0 - Сон (вимкнений)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
uint8_t bcd_to_dec(uint8_t bcd);
void read_time(uint8_t *h, uint8_t *m, uint8_t *s);
void init_oled(void);
void OLED_DisplayOnOff(uint8_t state);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

void read_time(uint8_t *h, uint8_t *m, uint8_t *s) {
    uint8_t data[3];

    HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR, 0x00,
                     I2C_MEMADD_SIZE_8BIT, data, 3, 50);

    *s = bcd_to_dec(data[0] & 0x7F);
    *m = bcd_to_dec(data[1]);
    *h = bcd_to_dec(data[2] & 0x3F);
}

void init_oled(void) {
    uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
        0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
        0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB,
        0x40, 0xA4, 0xA6, 0xAF
    };

    uint8_t control = 0x00;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, &control, 1, 50);
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, (uint8_t*)init_cmds,
                           sizeof(init_cmds), 50);
}

void OLED_DisplayOnOff(uint8_t state) {
    uint8_t cmd[2] = {0x00, state ? 0xAF : 0xAE};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, cmd, 2, 50);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  init_oled();
  lastMotionTime = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t currentMillis = HAL_GetTick();

    // 1. Перевірка таймауту неактивності: якщо рух відсутній довше заданого часу, гасимо дисплей
    if ((currentMillis - lastMotionTime > INACTIVITY_TIMEOUT_MS) && displayState) {
        OLED_DisplayOnOff(0); // Вимкнути дисплей (команда 0xAE)
        displayState = 0;
    }

    // 2. Якщо дисплей активний, зчитуємо час та виводимо його
    if (displayState) {
        read_time(&hour, &min, &sec);
        printf("%02d:%02d:%02d\n", hour, min, sec);
    }

    HAL_Delay(1000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000; // 100 кГц для коректної роботи з DS1307
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Налаштування піна PA0 для датчика руху PIR AM312 (переривання по наростаючому фронту)
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Увімкнення переривання EXTI0 в NVIC
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/* USER CODE BEGIN 4 */
/**
  * @brief Зовнішнє переривання від датчика руху AM312
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        lastMotionTime = HAL_GetTick(); // Скидання лічильника неактивності

        if (!displayState) {
            OLED_DisplayOnOff(1); // Увімкнути дисплей (команда 0xAF)
            displayState = 1;
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
