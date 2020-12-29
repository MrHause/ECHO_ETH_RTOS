/*
 * display.c
 *
 *  Created on: Dec 20, 2020
 *      Author: mrhause
 */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "queue.h"
#include "gpio.h"
#include <string.h>
#include "fonts.h"
#include "display.h"
#include "multicorecomm.h"

TaskHandle_t disp_task_handle = NULL;

QueueHandle_t display_queue;

void display_task(void const * argument);

void display_init(){
	display_queue = xQueueCreate(2, sizeof(disp_window_t));
	if(display_queue == NULL)
		return -1;

	xTaskCreate(display_task, "disp_task", 512, NULL, tskIDLE_PRIORITY + 4,
			&disp_task_handle);

	SSD1306_Init();
	SSD1306_Fill(0);
	SSD1306_UpdateScreen();
}

void display_task(void const * argument){
	while(1){
		disp_window_t window;
		MC_FRAME resp;
		mc_error_t ret;
		uint8_t str_buf[10] = {0};
		/*
		ret = mc_SendReceive(&resp, STAT_OK, GET_TEMP, NULL, 0);
		uint16_t t1 = 0, t2 = 0;
		memcpy(&t1, &resp.data[0], 2);
		memcpy(&t2, &resp.data[2], 2);
		sprintf(str_buf, "%d.%d C", t1, t2);
		SSD1306_Fill(0); //clear screen
		SSD1306_GotoXY(0, 10);
		SSD1306_Puts((char *)str_buf, &Font_7x10, 1);
		SSD1306_GotoXY(0, 22);
		SSD1306_DrawLine(0, 22, 127, 22, 1);
		SSD1306_DrawFilledCircle(64, 40, 15, 1);
		SSD1306_UpdateScreen();
		 */
	    if( xQueueReceive( display_queue, &window, ( TickType_t ) 100 ) == pdPASS )
	    {
	    	SSD1306_Fill(0); //clear screen
	    	if(window.scrollbar_en){
	    		SSD1306_DrawLine(10, 0, 10, 63, 1);
	    		SSD1306_DrawFilledTriangle(0, 10, 5, 0, 10, 10, 1);
	    		SSD1306_DrawFilledTriangle(0, 53, 5, 63, 10, 53, 1);
	    	}
	    	if(window.OK_button_en){
	    		SSD1306_GotoXY(110, 0);
	    		SSD1306_Puts("OK", &Font_7x10, 1);
	    	}
	    	if(window.OK_button_en){
	    		SSD1306_GotoXY(90, 52);
	    		SSD1306_Puts("BACK", &Font_7x10, 1);
	    	}
	    	if(window.list_el_num){
				for(uint8_t i = 0; i<LABELS_MAX_NUM; i++){
					SSD1306_GotoXY(13, (i*12));
					SSD1306_Puts(window.labels[i], &Font_7x10, 1);
				}
				SSD1306_DrawLine(13, 9+(window.list_curr_el*10), 90, 9+(window.list_curr_el*10), 1);
	    	}
	    	SSD1306_UpdateScreen();
	    }

		//vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}

void display_send(disp_window_t window){
	xQueueSend(display_queue, &window, (TickType_t)10);
}
