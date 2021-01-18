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
#include <stdio.h>
#include <string.h>

TaskHandle_t menu_task_handler = NULL;

windows_t currentWindow, previousWindow;

disp_window_t wMenu, wTemp, wHumidity, wPressure, wWeather, wSetAltitude;

void menu_task(void const * argument);
static void menu_window_init();
static void menu_setActiveWindow(windows_t window);

int menu_init(){
	menu_window_init();

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
			case KEY_OK:{
				uint8_t item = display_getActiveElement(wMenu);
				switch(item){
				case 0:
					menu_setActiveWindow(WIN_TEMPERATURE);
					break;
				case 1:
					menu_setActiveWindow(WIN_HUMIDITY);
					break;
				case 2:
					menu_setActiveWindow(WIN_PRESSURE);
					break;
				case 3:
					menu_setActiveWindow(WIN_WEATHER_PARAMS);
					//wSetAltitude.labels[3][0] = '0';
					//strcpy(wSetAltitude.labels[3], "0000");
					//menu_setActiveWindow(WIN_SET_ALTITUDE);
					break;
				default:
					break;
				}
				key_debouce();
				break;
			}
			case KEY_BACK:
				break;
			case KEY_UP:
					key_debouce();
					display_incrementUnderline(&wMenu);
				break;
			case KEY_DOWN:
					key_debouce();
					display_decrementUnderline(&wMenu);
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
				key_debouce();
				menu_setActiveWindow(WIN_MENU);
				break;
			case KEY_BACK:
				break;
			default:
				break;
			}
			break;
		}
		case WIN_HUMIDITY:{
			MC_FRAME resp;
			mc_error_t ret;
			ret = mc_SendReceive(&resp, STAT_OK, GET_HUM, NULL, 0);
			uint16_t h1 = 0, h2 = 0;
			memcpy(&h1, &resp.data[0], 2);
			memcpy(&h2, &resp.data[2], 2);
			sprintf(wHumidity.labels[4], "%d.%d", h1, h2);
			display_send(wHumidity);
			switch(key){
			case KEY_OK:
				key_debouce();
				menu_setActiveWindow(WIN_MENU);
				break;
			case KEY_BACK:
				break;
			default:
				break;
			}
			break;
		}
		case WIN_PRESSURE:{
			MC_FRAME resp;
			mc_error_t ret;
			ret = mc_SendReceive(&resp, STAT_OK, GET_PRESS, NULL, 0);
			int32_t pressure;
			//memcpy(&pressure, &resp.data[0], sizeof(pressure));
			//sprintf(wPressure.labels[4], "%ld Pa", pressure);
			uint16_t h1 = 0, h2 = 0;
			memcpy(&h1, &resp.data[0], 2);
			memcpy(&h2, &resp.data[2], 2);
			sprintf(wPressure.labels[4], "%d.%d", h1, h2);
			display_send(wPressure);
			switch(key){
			case KEY_OK:
				key_debouce();
				menu_setActiveWindow(WIN_MENU);
				break;
			case KEY_BACK:
				break;
			default:
				break;
			}
			break;
		}
		case WIN_WEATHER_PARAMS:{
			MC_FRAME resp;
			mc_error_t ret;
			ret = mc_SendReceive(&resp, STAT_OK, GET_WEATHER_PARAM, NULL, 0);
			//pars temperature
			uint16_t t1 = 0, t2 = 0;
			memcpy(&t1, &resp.data[0], 2);
			memcpy(&t2, &resp.data[2], 2);
			sprintf(wWeather.labels[1], "%d.%d C", t1, t2);
			//parse humidity
			uint16_t h1 = 0, h2 = 0;
			memcpy(&h1, &resp.data[4], 2);
			memcpy(&h2, &resp.data[6], 2);
			sprintf(wWeather.labels[2], "HUMIDITY: %d.%d", h1, h2);
			//parse presure
			int32_t pressure;
			memcpy(&pressure, &resp.data[8], sizeof(pressure));
			sprintf(wWeather.labels[4], "%ld Pa", pressure);
			display_send(wWeather);
			switch(key){
			case KEY_OK:
				key_debouce();
				menu_setActiveWindow(WIN_MENU);
				break;
			case KEY_BACK:
				break;
			default:
				break;
			}
			break;
		}
		case WIN_SET_ALTITUDE:{
			display_send(wSetAltitude); //refresh
			switch(key){
			case KEY_OK:
				key_debouce();
				menu_setActiveWindow(WIN_MENU);
				break;
			case KEY_BACK:
				break;
			case KEY_UP:
					key_debouce();
					display_incrementIndicator(&wSetAltitude);
					display_send(wSetAltitude);
				break;
			case KEY_DOWN:
					key_debouce();
					display_incrementIndicatedPosition(&wSetAltitude);
					display_send(wSetAltitude);
				break;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		vTaskDelay(200/portTICK_PERIOD_MS);
	}
}

static void menu_setActiveWindow(windows_t window){
	currentWindow = window;
}

static void menu_window_init(){
	//**********window menu******************
	memset(&wMenu, 0, sizeof(disp_window_t));
	wMenu.scrollbar_en = 1;
	wMenu.OK_button_en = 1;
	wMenu.BACK_button_en = 1;
	wMenu.list_curr_el = 0;
	wMenu.list_el_num = 4;
	wMenu.list_pointer_en = 1;
	strcpy(wMenu.labels[0], "GET TEMP.");
	strcpy(wMenu.labels[1], "GET HUMI.");
	strcpy(wMenu.labels[2], "GET PRESS.");
	strcpy(wMenu.labels[3], "ALL PARAMETERS");

	//*********window temperature************
	memset(&wTemp, 0, sizeof(disp_window_t));
	wTemp.scrollbar_en = 0;
	wTemp.OK_button_en = 0;
	wTemp.BACK_button_en = 1;
	wTemp.list_curr_el = 0;
	wTemp.list_el_num = 2;
	strcpy(wTemp.labels[3], "TEMPERATURE:");
	strcpy(wTemp.labels[4], "");

	//*********window humidity************
	memset(&wHumidity, 0, sizeof(disp_window_t));
	wHumidity.scrollbar_en = 0;
	wHumidity.OK_button_en = 0;
	wHumidity.BACK_button_en = 1;
	wHumidity.list_curr_el = 0;
	wHumidity.list_el_num = 2;
	strcpy(wHumidity.labels[3], "HUMIDITY:");
	strcpy(wHumidity.labels[4], "");

	//*********window pressure************
	memset(&wPressure, 0, sizeof(disp_window_t));
	wPressure.scrollbar_en = 0;
	wPressure.OK_button_en = 0;
	wPressure.BACK_button_en = 1;
	wPressure.list_curr_el = 0;
	wPressure.list_el_num = 2;
	strcpy(wPressure.labels[3], "PRESSURE:");
	strcpy(wPressure.labels[4], "");

	//**********WINDOW WEATHER*************
	memset(&wWeather, 0, sizeof(disp_window_t));
	wWeather.scrollbar_en = 0;
	wWeather.OK_button_en = 0;
	wWeather.BACK_button_en = 1;
	wWeather.list_curr_el = 0;
	wWeather.list_el_num = 6;
	strcpy(wWeather.labels[0], "TEMPERATURE:");
	strcpy(wWeather.labels[1], "");
	strcpy(wWeather.labels[2], "HUMIDITY:");
	strcpy(wWeather.labels[3], "PRESSURE:");
	strcpy(wWeather.labels[4], "");

	//*****WINDOW SET ALTITUTE***********
	memset(&wSetAltitude, 0, sizeof(disp_window_t));
	wSetAltitude.scrollbar_en = 0;
	wSetAltitude.OK_button_en = 1;
	wSetAltitude.BACK_button_en = 0;
	wSetAltitude.list_curr_el = 0;
	wSetAltitude.list_el_num = 2;
	strcpy(wSetAltitude.labels[0], "ALTITUDE:");
	wSetAltitude.labelsEditFocus = 3;
	wSetAltitude.labelsEdit[2] = 1;	//enable label 3 to be edited
	wSetAltitude.labelsEditIndicator = 0; //begining position of the indicator
	wSetAltitude.labelsEditMaxIndicator = 4; //maximum 4 position can be edited
}
