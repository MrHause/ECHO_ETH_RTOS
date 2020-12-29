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
#define CM7_to_CM4_ADDR 0x30040400

#define HSEM_SEND (8U)
#define HSEM_RECEIVE (9U)

typedef enum{
	LED2_ON,
	LED2_OFF,
	LED2_TOG,
	LED3_ON,
	LED3_OFF,
	LED3_TOG,
	GET_TEMP,
	GET_HUM,
	COMMAND_UNKNOWN
}MC_Commands;

typedef enum{
	STAT_OK,
	STAT_NOK,
	STAT_TIMEOUT
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

extern volatile MC_FRAME* CM4_to_CM7;
extern volatile MC_FRAME* CM7_to_CM4;

int mc_init();
mc_error_t mc_send(MC_Status stat, MC_Commands comm, uint8_t *buff, uint16_t buff_len );
mc_error_t mc_SendReceive(MC_FRAME *response, MC_Status stat, MC_Commands comm, uint8_t *buff, uint16_t buff_len);
mc_error_t mc_sendpacket(MC_FRAME packet);
void multicore_task(void const * argument);

static inline char *stringFromStatus(MC_Status stat)
{
    static const char *strings[] = { "Status: OK", "Status: NOK", "Status: Timeout"};

    return strings[stat];
}

#endif /* SRC_MULTICORECOMM_H_ */
