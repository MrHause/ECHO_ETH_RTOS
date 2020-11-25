/*
 * multicorecomm.h
 *
 *  Created on: 25 lis 2020
 *      Author: MrHause
 */

#ifndef SRC_MULTICORECOMM_H_
#define SRC_MULTICORECOMM_H_

#define MC_DATA_SIZE 32
#define CM4_to_CM7_ADDR 0x30040000
#define CM7_to_CM4_ADDR 0x30080000
enum MC_Commands{
	Command1,
	Command2,
	Command3
};

enum MC_Status{
	Stat1,
	Stat2,
	Stat3
};

struct MC_FRAME{
	enum MC_Status status :8;
	enum MC_Commands command :8;
	uint8_t dataLen;
	uint8_t data[32];
};

//volatile struct shared_data * const shared_ptr = (struct shared_data*)0x30040000;
volatile struct MC_FRAME* CM4_to_CM7 = (struct MC_FRAME*)CM4_to_CM7_ADDR;
volatile struct MC_FRAME* CM7_to_CM4 = (struct MC_FRAME*)CM7_to_CM4_ADDR;

volatile uint8_t* CM4_to_CM7_buff = (uint8_t *)CM4_to_CM7_ADDR;

#endif /* SRC_MULTICORECOMM_H_ */
