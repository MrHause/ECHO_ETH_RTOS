/*
 * keys.c
 *
 *  Created on: Dec 28, 2020
 *      Author: mrhause
 */

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "queue.h"
#include "gpio.h"
#include "keys.h"
#include <string.h>

QueueHandle_t keys_queue;

int keys_init(){
	keys_queue = xQueueCreate(2, sizeof(keys_t));
	if(keys_queue == NULL)
		return -1;

	return 0;
}

keys_t key_getKey(){
	keys_t key;

    if( xQueueReceive( keys_queue, &key, ( TickType_t ) 10 ) == pdPASS )
    {
       return key;
    }

	return KEY_NONE;
}

void key_debouce(){
	vTaskDelay(50/portTICK_PERIOD_MS);
	xQueueReset(keys_queue);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	keys_t key;
	BaseType_t xHigherPriorityTaskWoken;

	/* We have not woken a task at the start of the ISR. */
	xHigherPriorityTaskWoken = pdFALSE;
	static uint8_t one_shot_flag = 0;
	if(GPIO_Pin == GPIO_PIN_10){
		__HAL_GPIO_EXTID2_CLEAR_IT(GPIO_Pin);
		key = KEY_UP;
		xQueueSendFromISR( keys_queue, &key, &xHigherPriorityTaskWoken );
	}
	if(GPIO_Pin == GPIO_PIN_11){
		__HAL_GPIO_EXTID2_CLEAR_IT(GPIO_Pin);
		key = KEY_DOWN;
		xQueueSendFromISR( keys_queue, &key, &xHigherPriorityTaskWoken );
	}
	if(GPIO_Pin == GPIO_PIN_13){
		__HAL_GPIO_EXTID2_CLEAR_IT(GPIO_Pin);
		key = KEY_OK;
		xQueueSendFromISR( keys_queue, &key, &xHigherPriorityTaskWoken );	}
	if(GPIO_Pin == GPIO_PIN_14){
		__HAL_GPIO_EXTID2_CLEAR_IT(GPIO_Pin);
		key = KEY_BACK;
		xQueueSendFromISR( keys_queue, &key, &xHigherPriorityTaskWoken );
	}
    if( xHigherPriorityTaskWoken )
    {
        /* Actual macro used here is port specific. */
        //portYIELD_FROM_ISR();
    }
}
