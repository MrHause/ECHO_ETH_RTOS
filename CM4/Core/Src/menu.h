/*
 * menu.h
 *
 *  Created on: Dec 28, 2020
 *      Author: mrhause
 */

#ifndef SRC_MENU_H_
#define SRC_MENU_H_

typedef enum{
	WIN_MENU,
	WIN_TEMPERATURE,
	WIN_HUMIDITY,
	WIN_PRESSURE,
	WIN_NONE
}windows_t;

int menu_init();

#endif /* SRC_MENU_H_ */
