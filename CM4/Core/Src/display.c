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
#include "ssd1306.h"

TaskHandle_t disp_task_handle = NULL;

QueueHandle_t display_queue;

static void display_drawScrollBar();
static void display_drawUnderLine(disp_window_t window);
static void display_drawIndicator(disp_window_t window);
void display_task(void const * argument);

int display_init(){
	display_queue = xQueueCreate(2, sizeof(disp_window_t));
	if(display_queue == NULL)
		return -1;

	xTaskCreate(display_task, "disp_task", 512, NULL, tskIDLE_PRIORITY + 4,
			&disp_task_handle);

	SSD1306_Init();
	SSD1306_Fill(0);
	SSD1306_UpdateScreen();
	return 0;
}

void display_task(void const * argument){
	while(1){
		disp_window_t window;

	    if( xQueueReceive( display_queue, &window, ( TickType_t ) 100 ) == pdPASS )
	    {
	    	SSD1306_Fill(0); //clear screen
	    	if(window.scrollbar_en){
	    		display_drawScrollBar();
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
					if(window.labelsEdit[i]){ 		//if label is textedit
						display_drawIndicator(window);
					}
				}
	    	}
			if (window.list_pointer_en)
				display_drawUnderLine(window);

	    	SSD1306_UpdateScreen();
	    }

		//vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}

void display_send(disp_window_t window){
	xQueueSend(display_queue, &window, (TickType_t)10);
}

uint8_t display_getActiveElement(disp_window_t window){
	if( window.list_el_num )
		return window.list_curr_el;
	else
		return 0;
}

static void display_drawScrollBar(){
	SSD1306_DrawLine(10, 0, 10, 63, 1);
	SSD1306_DrawFilledTriangle(0, 10, 5, 0, 10, 10, 1);
	SSD1306_DrawFilledTriangle(0, 53, 5, 63, 10, 53, 1);
}

static void display_drawUnderLine(disp_window_t window){
	SSD1306_DrawLine(13, 9 + (window.list_curr_el * 12), 90, 9 + (window.list_curr_el * 12), 1);
}

static void display_drawIndicator(disp_window_t window){
	SSD1306_DrawLine(13+(window.labelsEditIndicator*7), 11 + (window.labelsEditFocus * 12), 20+(window.labelsEditIndicator*7), 11 + (window.labelsEditFocus * 12), 1);
}

void display_incrementUnderline(disp_window_t *window){
	if( window->list_el_num <= (window->list_curr_el+1) )
		window->list_curr_el = 0;
	else
		window->list_curr_el++;
}
void display_decrementUnderline(disp_window_t *window){
	if( (window->list_curr_el-1)<0 )
		window->list_curr_el = (window->list_el_num-1);
	else
		window->list_curr_el--;
}

void display_incrementIndicator(disp_window_t *window){
	if( window->labelsEditMaxIndicator <= (window->labelsEditIndicator+1) )
		window->labelsEditIndicator = 0;
	else
		window->labelsEditIndicator++;
}
void display_decrementIndicator(disp_window_t *window){
	if( (window->labelsEditIndicator-1)<0 )
		window->labelsEditIndicator = (window->labelsEditMaxIndicator-1);
	else
		window->labelsEditIndicator--;
}
void display_incrementIndicatedPosition(disp_window_t *window){
	uint8_t line = display_getLabelEditFocus(window);
	if((window->labels[line][window->labelsEditIndicator]+1)>='9')
		window->labels[line][window->labelsEditIndicator] = '0';
	else
		window->labels[line][window->labelsEditIndicator]++;
}
void display_decrementIndicatedPosition(disp_window_t *window){
	uint8_t line = display_getLabelEditFocus(window);
	if((window->labels[line][window->labelsEditIndicator]-1)<='0')
		window->labels[line][window->labelsEditIndicator] = '9';
	else
		window->labels[line][window->labelsEditIndicator]--;
}

void display_setLabelEditFocus(disp_window_t *window, uint8_t line){
	if(window->labelsEdit[line])
		window->labelsEditFocus = line;
}
uint8_t display_getLabelEditFocus(disp_window_t *window){
	return window->labelsEditFocus;
}
