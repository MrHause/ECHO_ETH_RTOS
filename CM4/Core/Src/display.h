/*
 * display.h
 *
 *  Created on: Dec 20, 2020
 *      Author: mrhause
 */

#ifndef SRC_DISPLAY_H_
#define SRC_DISPLAY_H_

#define LABELS_MAX_NUM 5 //number of elements in the list
#define LABALES_MAX_DISP_ELEM 4 //number of elements that display can handle
#define LABEL_MAX_BUF 20 //number of signs in label

typedef struct{
	uint8_t scrollbar_en;
	uint8_t OK_button_en;
	uint8_t BACK_button_en;
	uint8_t list_el_num;
	uint8_t list_curr_el;
	uint8_t labels[LABELS_MAX_NUM][LABEL_MAX_BUF];
	uint8_t list_pointer_en;
	uint8_t labelsEdit[LABELS_MAX_NUM]; //which labels are enabled to edit
	uint8_t labelsEditFocus;			//set focus to label to be edited
	uint8_t labelsEditIndicator; 	    //current element to edit
	uint8_t labelsEditMaxIndicator;     //number of the elements that can be edited
}disp_window_t;

int display_init();

void display_send(disp_window_t window);
uint8_t display_getActiveElement(disp_window_t window);
void display_incrementUnderline(disp_window_t *window);
void display_decrementUnderline(disp_window_t *window);
void display_incrementIndicator(disp_window_t *window);
void display_decrementIndicator(disp_window_t *window);
void display_incrementIndicatedPosition(disp_window_t *window);
void display_decrementIndicatedPosition(disp_window_t *window);
void display_setLabelEditFocus(disp_window_t *window, uint8_t line);
uint8_t display_getLabelEditFocus(disp_window_t *window);
#endif /* SRC_DISPLAY_H_ */
