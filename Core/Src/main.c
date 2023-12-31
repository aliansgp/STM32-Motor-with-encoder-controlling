/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "LCD16x2.h"
#include "LCD16x2_cfg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define IDLE   0
#define BUSY   1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

//int raw_count_number = 0;

uint32_t one_minute_counter = 0, one_minute_flag = 0;

uint16_t adc_raw_value = 0, adc_raw_value_percentage = 0;

uint32_t F_CLK = 1000000; // Reference frequency ( = 1 Mhz )
uint8_t State = IDLE; // Determines if input signal is 0 or 1 at the moment
uint32_t T1 = 0;
uint32_t T2 = 0;
uint32_t Ticks = 0; // Number of counts between two rising edges
uint16_t OVC = 0; // Overflow counter
uint32_t Freq = 0; // Raw frequency
uint32_t Feed_back_speed = 0; // Result (Feed back speed)
uint32_t maximum_Feed_back_speed = 0;
uint32_t Feed_back_speed_percentage = 0;

char Feed_back_speed_text[5], adc_raw_value_text[5],  Feed_back_speed_percentage_text[5],  adc_raw_value_percentage_text[5];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim)
{
	if(State == IDLE)
	{
		T1 = TIM1->CCR1;
		OVC = 0;
		State = BUSY;
	}
	else if(State == BUSY)
	{
		T2 = TIM1->CCR1;
		Ticks = (T2 + (OVC * 65535)) - T1;
		Freq = (uint32_t)(F_CLK/Ticks);
		Feed_back_speed = Freq * 60 ; // Converting frequency to rpm
		State = IDLE;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	if ( htim == &htim1 )
		OVC++;

	if ( htim == &htim3 )// 1ms
	{
		if ( one_minute_flag == 1)
		{
			if ( adc_raw_value_percentage > Feed_back_speed_percentage)
				TIM2 -> CCR1 ++;
			if ( adc_raw_value_percentage < Feed_back_speed_percentage)
				TIM2 -> CCR1 --;
		}

		one_minute_counter++;
		if (one_minute_counter == 60000)// 1 minute has passed
			one_minute_flag = 1;
	}
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
	MX_TIM1_Init();
	MX_ADC1_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	/* USER CODE BEGIN 2 */

	//ADC start
	HAL_ADC_Start(&hadc1);

	//PWM start
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

	//Input Capture start
	HAL_TIM_Base_Start_IT(&htim1);
	HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);

	//LCD initialization
	LCD_Init();
	LCD_Clear();
	LCD_Set_Cursor(1, 1);
	LCD_Write_String(" ");

	//1 minute timer start
	HAL_TIM_Base_Start_IT(&htim3);

	//First step
	TIM2 -> CCR1 = 99;
	sprintf(Feed_back_speed_text,"%lud",Feed_back_speed);
	LCD_Set_Cursor(1, 2);
	LCD_Write_String(Feed_back_speed_text);


	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		adc_raw_value = HAL_ADC_GetValue(&hadc1);

		LCD_Clear();

		Feed_back_speed_percentage = (Feed_back_speed / maximum_Feed_back_speed) * 100;
		sprintf(Feed_back_speed_text,"%lud",Feed_back_speed);
		LCD_Set_Cursor(1, 2);
		LCD_Write_String(Feed_back_speed_text);


		if ( one_minute_flag == 0 )
		{
			maximum_Feed_back_speed = Feed_back_speed;
		}

		if ( one_minute_flag == 1 )
		{
			LCD_Clear();

			sprintf(Feed_back_speed_percentage_text,"%lud",Feed_back_speed_percentage);
			LCD_Set_Cursor(1, 10);
			LCD_Write_String(Feed_back_speed_percentage_text);

			sprintf(adc_raw_value_text,"%d",adc_raw_value);
			LCD_Set_Cursor(2, 2);
			LCD_Write_String(adc_raw_value_text);
			sprintf(adc_raw_value_percentage_text,"%d",adc_raw_value_percentage);
			LCD_Set_Cursor(2, 10);
			LCD_Write_String(adc_raw_value_percentage_text);
		}

		HAL_Delay(20);
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

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		Error_Handler();
	}

	/** Enables the Clock Security System
	 */
	HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */

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
