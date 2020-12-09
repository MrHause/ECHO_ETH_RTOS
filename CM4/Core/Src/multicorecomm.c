/*
 * multicorecomm.c
 *
 *  Created on: Nov 30, 2020
 *      Author: mrhause
 */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "queue.h"
#include "gpio.h"
#include "multicorecomm.h"
#include <string.h>
//#include "FreeRTOS.h"
//#include "task.h"



volatile MC_FRAME* CM4_to_CM7 = (MC_FRAME*)CM4_to_CM7_ADDR;
volatile MC_FRAME* CM7_to_CM4 = (MC_FRAME*)CM7_to_CM4_ADDR;

SemaphoreHandle_t new_msg_sem;

QueueHandle_t mc_queue;

TaskHandle_t mc_task_handle = NULL;

MC_FRAME mc_frame_prepare( MC_Status stat, MC_Commands comm, uint8_t *buff, uint16_t buff_len );
static void mc_send_notification();

int MC_Init(){
	//create task
	//if(xTaskCreate(multicore_task, "mcTask", 512, NULL, tskIDLE_PRIORITY+4, &mc_task_handle) != pdPASS )
	//	return -1;

	//create binary semaphore
	new_msg_sem = xSemaphoreCreateBinary();
	if(new_msg_sem == NULL)
		return -2;

	mc_queue = xQueueCreate(2, sizeof(MC_FRAME));
	if(mc_queue == NULL)
		return -3;

	sys_thread_new("mc_thread", multicore_task, NULL, 512, tskIDLE_PRIORITY + 4);


	//enable notification for incomming answers from CM7
	HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_RECEIVE));

	return 0;
}

void multicore_task(void const * argument){
	MC_FRAME package;
	uint8_t buff[20];
	//MC_Init();
	while(1){
		//wait for semaphore

		//HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		if(xSemaphoreTake(new_msg_sem, 500) == pdTRUE){
			HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);

			memcpy( &package, CM7_to_CM4, sizeof(package) ); //copy answer from CM7
			xQueueSend(mc_queue, (void *)&package, (TickType_t)20);
		}
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}

mc_error_t SendPacket(MC_FRAME packet){
	uint8_t buff[20];

	memset(&packet, 0, sizeof(packet));
	sprintf(buff, "hello CM4\n");
	packet.status = Stat2;
	packet.command = Command2;
	packet.dataLen = sizeof(buff);
	memcpy(packet.data, buff, strlen(buff));
	memcpy(CM4_to_CM7, &packet, sizeof(packet)+packet.dataLen);

	HAL_HSEM_FastTake(HSEM_SEND);
	HAL_HSEM_Release(HSEM_SEND, 0);

	return MC_OK;
}

mc_error_t mc_send(MC_Status stat, MC_Commands comm, uint8_t *buff, uint16_t buff_len){
	MC_FRAME packet;
	packet = mc_frame_prepare(stat, comm, buff, buff_len); //prepare frame

	memcpy(CM4_to_CM7, &packet, sizeof(packet)+packet.dataLen);	//copy frame to shared memory

	mc_send_notification();

	return MC_OK;
}

MC_FRAME mc_frame_prepare( MC_Status stat, MC_Commands comm, uint8_t *buff, uint16_t buff_len ){
	MC_FRAME mc_frame;

	mc_frame.status = stat;
	mc_frame.command = comm;
	mc_frame.dataLen = buff_len;
	memcpy(mc_frame.data, buff, buff_len);

	return mc_frame;
}
static void mc_send_notification(){
	HAL_HSEM_FastTake(HSEM_SEND);
	HAL_HSEM_Release(HSEM_SEND, 0);
}

mc_error_t mc_SendReceive(MC_FRAME *response, MC_Status stat, MC_Commands comm, uint8_t *buff, uint16_t buff_len){
	//send
	MC_FRAME packet;
	packet = mc_frame_prepare(stat, comm, buff, buff_len); //prepare frame

	memcpy(CM4_to_CM7, &packet, sizeof(packet)+packet.dataLen);	//copy frame to shared memory

	mc_send_notification(); //send interrupt to second core
	if( xQueueReceive(mc_queue, response, 5000/portTICK_PERIOD_MS ) == pdPASS ){
		return MC_OK;
	}else
		return MC_TIMEOUT;
}

void HAL_HSEM_FreeCallback(uint32_t SemMask)
{

	if((SemMask &  __HAL_HSEM_SEMID_TO_MASK(HSEM_RECEIVE))!= 0){
		HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_RECEIVE));
		//give semaphore to mc task

		//xsemaphoregivefromisr !!!!
		//!!!!!!!!!!!!!!!!!!!!!!!!!!
		//xSemaphoreGive(new_msg_sem);
		//static unsigned char ucLocalTickCount = 0;
		static BaseType_t xHigherPriorityTaskWoken;

		/* A timer tick has occurred. */

		/* Is it time for vATask() to run? */
		xHigherPriorityTaskWoken = pdFALSE;

		/* Unblock the task by releasing the semaphore. */
		xSemaphoreGiveFromISR(new_msg_sem, &xHigherPriorityTaskWoken);

		/* If xHigherPriorityTaskWoken was set to true you
		 we should yield.  The actual macro used here is
		 port specific. */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

}
/*
void HAL_HSEM_FreeCallback(uint32_t SemMask)
{

	if((SemMask &  __HAL_HSEM_SEMID_TO_MASK(HSEM_RECEIVE))!= 0){
		HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_RECEIVE));
		//give semaphore to mc task
		xSemaphoreGive(new_msg_sem);
	}

}
*/

