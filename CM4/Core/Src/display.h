/*
 * display.h
 *
 *  Created on: Dec 20, 2020
 *      Author: mrhau
 */

#ifndef SRC_DISPLAY_H_
#define SRC_DISPLAY_H_

#define LABELS_MAX_NUM 5
#define LABEL_MAX_BUF 20

typedef struct{
	uint8_t scrollbar_en;
	uint8_t OK_button_en;
	uint8_t BACK_button_en;
	uint8_t list_el_num;
	uint8_t list_curr_el;
	uint8_t labels[LABELS_MAX_NUM][LABEL_MAX_BUF];
}disp_window_t;

void display_init();

void display_send(disp_window_t window);

#endif /* SRC_DISPLAY_H_ */
