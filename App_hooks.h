/*
 * AO_hooks.h
 *
 *  Created on: Jul 30, 2021
 *      Author: Samo Novak
 */

#ifndef INC_APP_HOOKS_H_
#define INC_APP_HOOKS_H_

#include <ActiveObject.h>
#include <stdlib.h>


enum Signals{
	TIMEOUT_SIG=USER_SIG,
//ethernet:
	ETHERNET_READ_SIG,
	SEND_IP_PACKET,
	SEND_TCP_PACKET,
//IP:
	RECEIVING_IP_PACKET,
	APP_PARAM_PROBLEM,
//TCP server:
	RECEIVING_TCP_PACKET,
	RECEIVING_APP_PARAMETER,
//Application:
	SEND_ANOTHER_PARAMETER,
	INCOMING_OPTION,
	BATTERY_CHECK,
	ANGLE_CHECK,
	MOTOR_CONTROL,
};

uint8_t ISR1_count;
uint16_t adc_buf[2];		//buffer used by DMA-ADC for battery status check

extern ADC_HandleTypeDef hadc1;

extern Active *ETHERNET_ptr;
extern Active *IP_LAYER_ptr;
extern Active *TCP_SERVER_ptr;
extern Active *APP_LAYER_ptr;
extern Active *APPLICATION_ptr;

int32_t constrain(int32_t x, int32_t high, int32_t low);

#endif /* INC_APP_HOOKS_H_ */
