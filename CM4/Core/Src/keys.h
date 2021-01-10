/*
 * keys.h
 *
 *  Created on: Dec 28, 2020
 *      Author: mrhause
 */

#ifndef SRC_KEYS_H_
#define SRC_KEYS_H_

typedef enum{
	KEY_OK,
	KEY_BACK,
	KEY_UP,
	KEY_DOWN,
	KEY_NONE
}keys_t;

int key_init();
keys_t key_getKey();
void key_debouce();
#endif /* SRC_KEYS_H_ */
