/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
#include "light_sensor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LIGHT_THRESHOLD 25 // How dim it must be (in lux) to turn relay on.
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
I2C_HandleTypeDef *g_hi2c1;
uint8_t timeConfigState = 0;
uint8_t wasSleeping = 0;
uint32_t lastButtonInt = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
void TimeConfiguration();
void ReportLux(long lux);
void IncrementTime(RTC_TimeTypeDef *time, uint8_t hoursDelta, uint8_t minutesDelta, uint8_t secondsDelta);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief State machine to configure the clock.
 */
void TimeConfiguration()
{
	HAL_StatusTypeDef status = HAL_OK;
	char msg[40];
	RTC_TimeTypeDef time;
	int i;

	time.Hours = 0;
	time.Minutes = 0;
	time.Seconds = 0;
	time.TimeFormat = RTC_HOURFORMAT12_AM;
	time.SubSeconds = time.SecondFraction = 0;
	time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	time.StoreOperation = RTC_STOREOPERATION_SET;

	// Turn on LED to indicate we're in configuration mode.
	HAL_GPIO_WritePin(ConfigLED_GPIO_Port, ConfigLED_Pin, GPIO_PIN_SET);

	while (timeConfigState < 3)
	{
		if (timeConfigState == 1) // Counting presses of BTN2 to increment the hour.
		{
			if (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == GPIO_PIN_SET)
			{
				HAL_Delay(200); // debounce

				time.Hours = (time.Hours + 1) % 24;

				sprintf(msg, "Incrementing hour to %i\r\n", time.Hours);
				HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);
			}
		}
		else if (timeConfigState == 2) // Counting presses of BTN2 to increment the minutes.
		{
			if (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == GPIO_PIN_SET)
			{
				HAL_Delay(200); // debounce

				time.Minutes = (time.Minutes + 1) % 60;

				sprintf(msg, "Incrementing minute to %i\r\n", time.Minutes);
				HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);
			}
		}
	}

	status = HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
	if (status == HAL_OK)
	{
		sprintf(msg, "Setting time to %i:%i\r\n", time.Hours, time.Minutes);
	}
	else
	{
		strcpy(msg, "Failed to set time.\r\n");
	}
	HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);

	timeConfigState = 0;

	// Blink LED to indicate completion
	for (i = 0; i < 5; i++)
	{
		HAL_GPIO_TogglePin(ConfigLED_GPIO_Port, ConfigLED_Pin);
		HAL_Delay(200);
	}
}

/**
 * @brief Flashes the config LED as many times as the integer log of lux.
 * @param lux The measured amount of lux.
 */
void ReportLux(long lux)
{
	long log = 0;
	for (; lux > 0;) {
		lux = lux / 10;
		if (lux > 0)
		{
			log++;
		}
	}

	for (long i = 0; i <= log; i++)
	{
		HAL_GPIO_WritePin(ConfigLED_GPIO_Port, ConfigLED_Pin, GPIO_PIN_SET);
		HAL_Delay(1000);
		HAL_GPIO_WritePin(ConfigLED_GPIO_Port, ConfigLED_Pin, GPIO_PIN_RESET);
		HAL_Delay(1000);
	}
}

/**
 * @brief Utility function to increment specific fields of TimeTypeDef struct.
 */
void IncrementTime(RTC_TimeTypeDef *time, uint8_t hoursDelta, uint8_t minutesDelta, uint8_t secondsDelta)
{
	time->Seconds += secondsDelta;
	if (time->Seconds > 60)
	{
		time->Seconds -= 60;
		time->Minutes++;
	}

	time->Minutes += minutesDelta;
	if (time->Minutes > 60)
	{
		time->Minutes -= 60;
		time->Hours++;
	}

	time->Hours = (time->Hours + hoursDelta) % 24;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	char msg[40];
	HAL_StatusTypeDef status = HAL_OK;
	long lux;
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
	RTC_AlarmTypeDef alarm;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
	g_hi2c1 = &hi2c1;

	strcpy(msg, "Christmas Lights 0.3\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);

	status = HAL_I2C_IsDeviceReady(&hi2c1, 0x29 << 1, 10, HAL_MAX_DELAY);
	if(status == HAL_OK)
	{
		strcpy(msg, "Light sensor ready\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);

		light_init();
	}
	else
	{
		sprintf(msg, "Light sensor not ready - error %i\r\n", status);
		HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);
	}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		if (timeConfigState)
		{
			strcpy(msg, "Entering time configuration\r\n");
			HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);

			TimeConfiguration();
		}
		else
		{
			status = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
			if (status == HAL_OK)
			  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN); // Must call this to unlock registers after GetTime

			lux = light_readVisibleLux();
			sprintf(msg, "%i:%i:%i Lux is %li\r\n", time.Hours, time.Minutes, time.Seconds, lux);
			HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);

			// Turn relay only after 8am if light drops below threshold.
			if (lux < LIGHT_THRESHOLD && time.Hours > 8)
			{
				HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET);
			}
			else
			{
				HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);
			}

			ReportLux(lux);

			// Setup an alarm to wake us up.
			// TODO: should be refactored into a separate function.
			HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
			alarm.AlarmMask = RTC_ALARMMASK_MINUTES;
			alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
			alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
			alarm.AlarmDateWeekDay = 1;
			alarm.Alarm = RTC_ALARM_A;

			if (time.Hours < 8) // Past midnight, set alarm for 8am
			{
				alarm.AlarmTime.Hours = 8;
				alarm.AlarmTime.Minutes = 0;
				alarm.AlarmTime.Seconds = 0;
			}
			else // Otherwise, alarm for 5 minutes from now.
			{
				alarm.AlarmTime = time;
				IncrementTime(&alarm.AlarmTime, 0, 5, 0);
			}

			sprintf(msg, "Going to sleep until %i:%i:%i\r\n",
					alarm.AlarmTime.Hours, alarm.AlarmTime.Minutes, alarm.AlarmTime.Seconds);
			HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 5000);

			status = HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);

			// Enter sleep mode. Must suspend SysTick otherwise its interrupts may wake us up.
			wasSleeping = 1;
			HAL_SuspendTick();
			HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		}
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  RTC_AlarmTypeDef sAlarm = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable the Alarm A
  */
  sAlarm.AlarmTime.Hours = 0;
  sAlarm.AlarmTime.Minutes = 0;
  sAlarm.AlarmTime.Seconds = 0;
  sAlarm.AlarmTime.SubSeconds = 0;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 1;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ConfigLED_GPIO_Port, ConfigLED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : RELAY_Pin */
  GPIO_InitStruct.Pin = RELAY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RELAY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ConfigLED_Pin */
  GPIO_InitStruct.Pin = ConfigLED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ConfigLED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN2_Pin */
  GPIO_InitStruct.Pin = BTN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN1_Pin */
  GPIO_InitStruct.Pin = BTN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BTN1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == BTN1_Pin)
	{
		// Resume clocks if the button press woke us up.
		if (wasSleeping)
		{
			SystemClock_Config();
			HAL_ResumeTick();
		}

		// Increment time configuration state, taking care to wait for button debounce.
		if (lastButtonInt == 0 || HAL_GetTick() - lastButtonInt > 200)
		{
			timeConfigState++;
		}
		lastButtonInt = HAL_GetTick();
	}
	wasSleeping = 0;
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	// Restore SysTick when the alarm wakes us up.
	SystemClock_Config();
	HAL_ResumeTick();
	wasSleeping = 0;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

