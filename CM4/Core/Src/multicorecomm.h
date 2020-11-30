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
typedef enum{
	Command1,
	Command2,
	Command3
}MC_Commands;

typedef enum{
	Stat1,
	Stat2,
	Stat3
}MC_Status;

typedef struct{
	MC_Status status;
	MC_Commands command;
	uint8_t dataLen;
	uint8_t data[32];
}MC_FRAME;

typedef enum{
	MC_OK,
	MC_ERROR,
	MC_TIMEOUT
}mc_error_t;
//volatile struct shared_data * const shared_ptr = (struct shared_data*)0x30040000;
volatile MC_FRAME* CM4_to_CM7 = (MC_FRAME*)CM4_to_CM7_ADDR;
volatile MC_FRAME* CM7_to_CM4 = (MC_FRAME*)CM7_to_CM4_ADDR;

volatile uint8_t* CM4_to_CM7_buff = (uint8_t *)CM4_to_CM7_ADDR;


int MC_Init();
#endif /* SRC_MULTICORECOMM_H_ */
