/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "multicorecomm.h"
#include "display.h"
#include <string.h>
#include <stdio.h>
#include "keys.h"
#include "menu.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
#define TCPECHO_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )

#define TCP_SERVER_ON 1
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void tcpecho_thread(void *arg);
void tcpecho_init(void);
void sending_init(void);
void sender_task(void *arg);

MC_Commands command_analyze(uint8_t *buff);
void prepare_response(MC_FRAME *resp, char *resp_buff);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  mc_init();
#ifdef TCP_SERVER_ON
  tcpecho_init();
#else
  sending_init();
#endif
  /*
  display_init();
  keys_init();
  menu_init();
  */
  /* Infinite loop */
  for(;;)
  {
	  osThreadTerminate(NULL);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void tcpecho_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  struct netbuf *buf;
  void *data;
  MC_Commands command;
  MC_FRAME package;
  char *response;
  char data_resp[32] = {0};
  u16_t len;

  LWIP_UNUSED_ARG(arg);

  /* Create a new connection identifier. */
  conn = netconn_new(NETCONN_TCP);

  if (conn!=NULL)
  {
    /* Bind connection to well known port number 7. */
    err = netconn_bind(conn, NULL, 7);

    if (err == ERR_OK)
    {
      /* Tell connection to go into listening mode. */
      netconn_listen(conn);

      while (1)
      {
        /* Grab new connection. */
         accept_err = netconn_accept(conn, &newconn);

        /* Process the new connection. */
        if (accept_err == ERR_OK)
        {

          while (netconn_recv(newconn, &buf) == ERR_OK)
          {
            do
            {
              netbuf_data(buf, &data, &len);
              uint8_t buf_check[20];
              memset(buf_check, 0, sizeof(buf_check));
              memset(data_resp, 0, sizeof(data_resp));
              memcpy(buf_check, buf->p->payload, buf->p->len);
              //***************
              command = command_analyze(buf_check); //analyze received command
              mc_SendReceive( &package, STAT_OK, command, NULL, 0); //send command to cm7 to be executed
              response = stringFromStatus( package.status );	//make string from status enum
              prepare_response(&package, data_resp);
              netconn_write(newconn, (void *)response, strlen(response), NETCONN_COPY); //send response
              netconn_write(newconn, (void *)data_resp, strlen(data_resp), NETCONN_COPY); //send response
              //***************
              //netconn_write(newconn, data, len, NETCONN_COPY);
            }
            while (netbuf_next(buf) >= 0);

            netbuf_delete(buf);
          }

          /* Close connection and discard connection identifier. */
          netconn_close(newconn);
          netconn_delete(newconn);
        }
        vTaskDelay(2);
      }
    }
    else
    {
      netconn_delete(newconn);
    }
  }
}

MC_Commands command_analyze(uint8_t *buff){

	MC_Commands command;

	if( !strcmp( buff, "LED2_ON" ) )
		command = LED2_ON;
	else if( !strcmp( buff, "LED2_OFF" ) )
		command = LED2_OFF;
	else if(!strcmp( buff, "LED2_TOG" ) )
		command = LED2_TOG;
	else if( !strcmp( buff, "LED3_ON" ) )
		command = LED3_ON;
	else if( !strcmp( buff, "LED3_OFF" ) )
		command = LED3_OFF;
	else if(!strcmp( buff, "LED3_TOG" ) )
		command = LED3_TOG;
	else if(!strcmp( buff, "GET_PARAMS" ) ){
		command = GET_WEATHER_PARAM;
	}
	else
		command = COMMAND_UNKNOWN;

	return command;
}

void prepare_response(MC_FRAME *resp, char *resp_buff){
	//char resp_buff[32] = {0};
	char temp_buff[32] = {0};
	uint16_t len;
	switch(resp->command){
	case GET_WEATHER_PARAM:{
		uint16_t t1 = 0, t2 = 0;
		memcpy(&t1, &resp->data[0], 2);
		memcpy(&t2, &resp->data[2], 2);
		sprintf(resp_buff, "%d.%d:", t1, t2);
		len = strlen(resp_buff);
		//parse humidity
		uint16_t h1 = 0, h2 = 0;
		memcpy(&h1, &resp->data[4], 2);
		memcpy(&h2, &resp->data[6], 2);
		sprintf(temp_buff, "%d.%d:", h1, h2);
		strcpy(resp_buff+len, temp_buff);
		len = strlen(resp_buff);
		memset(temp_buff, 0 ,sizeof(temp_buff));
		//parse presure
		int32_t pressure;
		memcpy(&pressure, &resp->data[8], sizeof(pressure));
		sprintf(temp_buff, "%ld:", pressure);
		strcpy(resp_buff+len, temp_buff);
		break;
	}
	default:
		break;
	}
}
/*-----------------------------------------------------------------------------------*/

void tcpecho_init(void)
{
  sys_thread_new("tcpecho_thread", tcpecho_thread, NULL, DEFAULT_THREAD_STACKSIZE, TCPECHO_THREAD_PRIO);
}
void sending_init(void){
	sys_thread_new("sending_thread", sender_task, NULL, 512, tskIDLE_PRIORITY + 4);
}

void sender_task(void *arg){
	MC_FRAME packet;
	MC_FRAME response;
	while(1){
		//SendPacket(packet);
		uint8_t buff[20];
		sprintf(buff, "hello CM4\n");

		//mc_send(Stat1, Command1, buff, strlen(buff));
		mc_SendReceive( &response, STAT_OK, LED2_TOG, buff, strlen(buff) );

		vTaskDelay(2000/portTICK_PERIOD_MS);
	}
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
