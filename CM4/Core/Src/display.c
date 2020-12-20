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

TaskHandle_t disp_task_handle = NULL;

void display_task(void const * argument);

void display_init(){
	xTaskCreate(display_task, "disp_task", 512, NULL, tskIDLE_PRIORITY + 4,
			&disp_task_handle);

	SSD1306_Init();
	SSD1306_Fill(0);
	SSD1306_UpdateScreen();
}

void display_task(void const * argument){
	while(1){
		SSD1306_Fill(0); //clear screen
		SSD1306_GotoXY(0, 10);
		SSD1306_Puts("Hello from STM", &Font_7x10, 1);
		SSD1306_GotoXY(0, 22);
		SSD1306_DrawLine(0, 22, 127, 22, 1);
		SSD1306_DrawFilledCircle(64, 40, 15, 1);
		SSD1306_UpdateScreen();

		vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}
