/*
 * menu.c
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
#include "menu.h"
#include "keys.h"
#include "display.h"
#include "multicorecomm.h"

TaskHandle_t menu_task_handler = NULL;

windows_t currentWindow, previousWindow;

disp_window_t wMenu, wTemp;

void menu_task(void const * argument);

int menu_init(){
	memset(&wMenu, 0, sizeof(disp_window_t));
	wMenu.scrollbar_en = 1;
	wMenu.OK_button_en = 1;
	wMenu.BACK_button_en = 1;
	wMenu.list_curr_el = 0;
	wMenu.list_el_num = 3;
	strcpy(wMenu.labels[0], "GET TEMP.");
	strcpy(wMenu.labels[1], "GET HUMI.");
	strcpy(wMenu.labels[2], "GET PRESS.");

	memset(&wTemp, 0, sizeof(disp_window_t));
	wTemp.scrollbar_en = 0;
	wTemp.OK_button_en = 0;
	wTemp.BACK_button_en = 1;
	wTemp.list_curr_el = 0;
	wTemp.list_el_num = 2;
	strcpy(wTemp.labels[3], "TEMPERATURE:");
	strcpy(wTemp.labels[4], "");

	xTaskCreate(menu_task, "menu_task", 256, NULL, tskIDLE_PRIORITY + 4, &menu_task_handler);

	currentWindow = WIN_MENU;
	previousWindow = WIN_NONE;

	return 0;
}


void menu_task(void const * argument){
	keys_t key;
	while(1){
		key = key_getKey();
		switch( currentWindow ){
		case WIN_MENU:

			display_send(wMenu);
			switch(key){
			case KEY_OK:
				if(wMenu.list_curr_el == 0)
					currentWindow = WIN_TEMPERATURE;
				break;
			case KEY_BACK:
				break;
			default:
				break;
			}
			break;
		case WIN_TEMPERATURE:{
			MC_FRAME resp;
			mc_error_t ret;
			uint8_t str_buf[10] = {0};
			ret = mc_SendReceive(&resp, STAT_OK, GET_TEMP, NULL, 0);
			uint16_t t1 = 0, t2 = 0;
			memcpy(&t1, &resp.data[0], 2);
			memcpy(&t2, &resp.data[2], 2);
			sprintf(wTemp.labels[4], "%d.%d C", t1, t2);
			display_send(wTemp);
			switch(key){
			case KEY_OK:
				currentWindow = WIN_MENU;
				break;
			case KEY_BACK:
				break;
			default:
				break;
			}
			break;
		}
		case WIN_HUMIDITY:
			break;
		case WIN_PRESSURE:
			break;
		default:
			break;
		}
		vTaskDelay(200/portTICK_PERIOD_MS);
	}
}
