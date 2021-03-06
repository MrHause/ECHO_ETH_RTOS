/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "eth.h"
#include "i2c.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "multicorecomm.h"
#include "BME280.h"
#include <math.h>
#include "FLASH_SECTOR.h"
#include "stdlib.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#define HSEM_NEW_MSG (7U)
#define HSEM_ID_8 (8U)
#define HSEM_SEND (9U)
#define ALTITUDE_MAGIC 0xfecd05ff
#define ALTITUDE_INIT 0x0000012c
uint8_t SEM_NEW_MSG = 0;
uint32_t flash_values[9] = {ALTITUDE_MAGIC, ALTITUDE_INIT};
__IO uint32_t RX_Data[2];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
MC_Status command_execute(MC_FRAME frame);
void send_notification();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

/* USER CODE BEGIN Boot_Mode_Sequence_0 */
  int32_t timeout;
/* USER CODE END Boot_Mode_Sequence_0 */

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
  Error_Handler();
  }
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
/* USER CODE BEGIN Boot_Mode_Sequence_2 */
	/* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
	 HSEM notification */
	/*HW semaphore Clock enable*/
	__HAL_RCC_HSEM_CLK_ENABLE();
	/*Take HSEM */
	HAL_HSEM_FastTake(HSEM_ID_0);
	/*Release HSEM in order to notify the CPU2(CM4)*/
	HAL_HSEM_Release(HSEM_ID_0, 0);
	/* wait until CPU2 wakes up from stop mode */
	timeout = 0xFFFF;
	while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0))
		;
	if (timeout < 0) {
		Error_Handler();
	}
/* USER CODE END Boot_Mode_Sequence_2 */

  /* USER CODE BEGIN SysInit */
	HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_8));
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  BME280_init(&hi2c2, BME280_TEMPERATURE_20BIT, BME280_PRESSURE_HIGHRES, BME280_HUMINIDITY_STANDARD, BME280_NORMALMODE);

  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  HAL_NVIC_SetPriority(CM4_SEV_IRQn, 0x0F, 0);
  HAL_NVIC_EnableIRQ(CM4_SEV_IRQn);
  HAL_NVIC_SetPriority(HSEM1_IRQn, 0x0F, 0);
  HAL_NVIC_EnableIRQ(HSEM1_IRQn);

  HAL_HSEM_FastTake(HSEM_NEW_MSG); //take sem and wait for release it in the interrupt callback

  MC_FRAME package;
  MC_FRAME response;
  HAL_FLASHEx_Unlock_Bank1();
  Flash_Read_Data(0x080E0000, RX_Data, 2);
  HAL_FLASHEx_Lock_Bank1();
	if (RX_Data[0] != ALTITUDE_MAGIC) { //flash not initialized
		HAL_FLASHEx_Unlock_Bank1();
		//Flash_Write_Data(0x080E0000, flash_values); //write magic value and init altitude for pressure meassure
		BME280_setAltitude(ALTITUDE_INIT);
		HAL_FLASHEx_Lock_Bank1();
	} else {
		BME280_setAltitude((uint32_t)RX_Data[1]); //set read altitude.
	}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  //if( HAL_HSEM_FastTake(HSEM_NEW_MSG) == HAL_OK ){
	  if( SEM_NEW_MSG ){ //if new message rexeived from CM4
		  uint8_t buff[20];
		  memcpy( &package, CM4_to_CM7, sizeof(package) ); //copy message from shared memory to local structure
		  MC_Status stat;
		  stat = command_execute(package); //execute command
		  send_notification(); //send notification to CM4 about new response. that generates interrupt in second core

		  SEM_NEW_MSG = 0; //clear flag about new message
	  }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_I2C2
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_USB;
  PeriphClkInitStruct.PLL3.PLL3M = 1;
  PeriphClkInitStruct.PLL3.PLL3N = 24;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 4;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable USB Voltage detector
  */
  HAL_PWREx_EnableUSBVoltageDetector();
}

/* USER CODE BEGIN 4 */
void HAL_HSEM_FreeCallback(uint32_t SemMask)
{

	if((SemMask &  __HAL_HSEM_SEMID_TO_MASK(HSEM_ID_8))!= 0){
		HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_8));
		//HAL_HSEM_Release(HSEM_NEW_MSG, 0);
		if( !SEM_NEW_MSG )
			SEM_NEW_MSG = 1; //set flag that new msg has been received for main loop
	}

}
MC_Status command_execute(MC_FRAME frame){
	MC_Commands command = frame.command;
	MC_FRAME resp;
	memset(&resp, 0, sizeof(MC_FRAME));
	if( command == LED2_ON ){
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 1);
		resp.command = command;
		resp.status = STAT_OK;
		resp.dataLen = 0;
		memcpy(CM7_to_CM4, &resp, sizeof(resp));
	}
	else if( command == LED2_OFF ){
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 0);
		resp.command = command;
		resp.status = STAT_OK;
		resp.dataLen = 0;
		memcpy(CM7_to_CM4, &resp, sizeof(resp));
	} else if (command == LED2_TOG) {
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		resp.command = command;
		resp.status = STAT_OK;
		resp.dataLen = 0;
		memcpy(CM7_to_CM4, &resp, sizeof(resp));
	} else if (command == LED3_ON) {
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 1);
		resp.command = command;
		resp.status = STAT_OK;
		resp.dataLen = 0;
		memcpy(CM7_to_CM4, &resp, sizeof(resp));
	} else if (command == LED3_OFF) {
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 0);
		resp.command = command;
		resp.status = STAT_OK;
		resp.dataLen = 0;
		memcpy(CM7_to_CM4, &resp, sizeof(resp));
	} else if (command == LED3_TOG) {
		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		resp.command = command;
		resp.status = STAT_OK;
		resp.dataLen = 0;
		memcpy(CM7_to_CM4, &resp, sizeof(resp));
	}
	else if ( command == GET_TEMP){
		float temperature,humidity;
		int32_t pressure;
		BME280_GetAll(&temperature, &pressure, &humidity);
		MC_FRAME response;
		response.status = STAT_OK;
		response.command = command;
		uint16_t t1 = (uint16_t)temperature;
		uint16_t temp = (uint16_t)(temperature*100);
		uint16_t t2 = (uint16_t)(temp%100);
		//sprintf(buff, "CM7 says Hi\n");
		memcpy(response.data, &t1, sizeof(t1));
		memcpy(response.data + sizeof(t1), &t2, sizeof(t2));
		response.dataLen = sizeof(t1) + sizeof(t2);
		memcpy(CM7_to_CM4, &response, sizeof(response)); //copy response to the shared memory
	} else if (command == GET_HUM) {
		float temperature, humidity;
		int32_t pressure;
		BME280_GetAll(&temperature, &pressure, &humidity);
		MC_FRAME response;
		response.status = STAT_OK;
		response.command = command;
		uint16_t h1 = (uint16_t) humidity;
		uint16_t temp = (uint16_t) (humidity * 100);
		uint16_t h2 = (uint16_t) (temp % 100);
		memcpy(response.data, &h1, sizeof(h1));
		memcpy(response.data + sizeof(h1), &h2, sizeof(h2));
		response.dataLen = sizeof(h1) + sizeof(h2);
		memcpy(CM7_to_CM4, &response, sizeof(response)); //copy response to the shared memory
	} else if (command == GET_PRESS) {
		float temperature, humidity,pressure_mnpm;
		int32_t pressure;
		BME280_GetAll(&temperature, &pressure, &humidity);
		BME280_GetPressure2(&pressure, &temperature, &pressure_mnpm);
		uint16_t h1 = (uint16_t) pressure_mnpm;
		uint16_t temp = (uint16_t) (pressure_mnpm * 100);
		uint16_t h2 = (uint16_t) (temp % 100);

		MC_FRAME response;
		response.status = STAT_OK;
		response.command = command;
		memcpy(response.data, &h1, sizeof(h1));
		memcpy(response.data + sizeof(h1), &h2, sizeof(h2));
		response.dataLen = sizeof(h1) + sizeof(h2);
		//memcpy(response.data, &pressure_mnpm, sizeof(pressure_mnpm));
		//response.dataLen = sizeof(pressure_mnpm);
		memcpy(CM7_to_CM4, &response, sizeof(response)); //copy response to the shared memory
	}else if (command == GET_WEATHER_PARAM) {
		float temperature, humidity;
		int32_t pressure;
		BME280_GetAll(&temperature, &pressure, &humidity);
		MC_FRAME response;
		response.status = STAT_OK;
		response.command = command;
		uint16_t t1 = (uint16_t)temperature;
		uint16_t temp = (uint16_t) (temperature * 100);
		uint16_t t2 = (uint16_t) (temp % 100);
		memcpy(response.data, &t1, sizeof(t1));
		memcpy(response.data + sizeof(t1), &t2, sizeof(t2));
		uint16_t h1 = (uint16_t) humidity;
		temp = (uint16_t) (humidity * 100);
		uint16_t h2 = (uint16_t) (temp % 100);
		memcpy(response.data + sizeof(t1) + sizeof(t2), &h1, sizeof(h1));
		memcpy(response.data + sizeof(t1) + sizeof(t2) + sizeof(h1), &h2,sizeof(h2));
		memcpy(response.data + sizeof(t1) + sizeof(t2) + sizeof(h1)+ sizeof(h2), &pressure, sizeof(pressure));
		response.dataLen = sizeof(t1) + sizeof(t2) + sizeof(h1) + sizeof(h2) + sizeof(pressure);
		memcpy(CM7_to_CM4, &response, sizeof(response)); //copy response to the shared memory
	}else if( command == SET_ALTITUDE){
		uint8_t buff[10];
		memcpy(buff, frame.data, frame.dataLen);
		int altitude = atoi(buff);
		uint32_t to_write[9] = {0};
		to_write[0] = ALTITUDE_MAGIC;
		to_write[1] = (uint32_t)altitude;
		/*
		HAL_FLASHEx_Unlock_Bank1();
		Flash_Write_Data(0x080E0000, to_write); //write magic value and init altitude for pressure meassure
		BME280_setAltitude((uint32_t)altitude);
		HAL_FLASHEx_Lock_Bank1();
		*/
		BME280_setAltitude((uint32_t)altitude);

		MC_FRAME response;
		memset(&response, 0, sizeof(response));
		response.status = STAT_OK;
		response.command = command;
		response.dataLen = 0;
		memcpy(CM7_to_CM4, &response, sizeof(response));
	}else
		return STAT_NOK;

	return STAT_OK;
}

void send_notification(){
	  HAL_HSEM_FastTake(HSEM_SEND);
	  HAL_HSEM_Release(HSEM_SEND, 0);
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
