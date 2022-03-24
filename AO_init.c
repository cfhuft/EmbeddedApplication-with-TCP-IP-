/*
 * AO_init.c
 *
 *  Created on: Jul 30, 2021
 *      Author: Samo Novak
 */

#include "AO_init.h"

static uint8_t TCP_parameters_queue [1000];	//IP-TCP queue for TCP packets
StaticQueue_t TCP_param_queue_cb;			//IP-TCP queue control block
QueueHandle_t TCP_param_queue;				//queue handle

static uint8_t APP_INPUT_queue [16];		//APP queue for parameters
StaticQueue_t APP_IN_queue_cb;				//APP queue control block
QueueHandle_t APP_IN_queue;					//queue handle

static uint8_t APP_OUTPUT_queue [16];		//APP queue for parameters
StaticQueue_t APP_OUT_queue_cb;				//APP queue control block
QueueHandle_t APP_OUT_queue;				//queue handle


typedef struct {
    Active super;					// inherit Active base class
    uint8_t ISR1_check;
    PacketEvent * ptr_e;
} ETHERNET;

typedef struct {
    Active super;
    PacketEvent * ptr_e;
} IP_LAYER;

typedef struct {
    Active super;
    TimeEvent te;
    bool socket_syn;				//socket synchronized? status
    bool socket;					//socket status
    int32_t out_parameter;			//parameter to send out to IP
    uint32_t saved_SEQ_number;		//TCP saved SEQ number for retransmission
    uint32_t saved_ACK_number;		//TCP saved ACK number for retransmission
    uint16_t client_port; 			//setting a client port after creating a socket
    uint32_t client_ip;				//setting a client IP after creating a socket
    uint8_t retransmission;			//retransmission counter

    //incoming packet stats:
    uint32_t source_ip;
    uint16_t source_port;
    uint32_t SEQ_number;
    uint32_t ACK_number;
    bool SEQ_ACK_state;				//state of SEQ and ACK numbers for TCP transmission
    uint8_t flag;					//TCP packet flag
    uint32_t in_parameter;			//parameter to send to APP
} TCP_SERVER;

typedef struct {
    Active super;
    TimeEvent te;
	int16_t PWM;
	uint8_t option[3];
	int16_t parameter;
} APPLICATION;

//----------------------------------------------------------------------------------------- ETHERNET ACTIVE OBJECT

static void ETHERNET_dispatch(ETHERNET * const me, Event * const e) {

	static PacketEvent ETHERNET_Packet_Event = {.Signal.sig=RECEIVING_IP_PACKET};		//event signal with parameter Packet[] (used for sending a packet to IP)
	me->ptr_e = (PacketEvent*)&e->sig;													//assign a pointer to PacketEvent struct to current event e

    switch (e->sig) {
    	case INIT_SIG:{break;}
    	case ETHERNET_READ_SIG:{

    		while(readEPKTCNT!=0 || me->ISR1_check!=ISR1_count){						//is buffer not empty?(variable only updates in ReadPacket()) || is ISR1
    																					//counter not equal (higher) than ISR1 count checker?
    			me->ISR1_check=ISR1_count;
    			ReadPacket(ETHERNET_Packet_Event.Packet);								//read ENC28 buffer
    			Active_post(IP_LAYER_ptr, (Event *)&ETHERNET_Packet_Event.Signal.sig);	//post event to IP_LAYER
    			}
    		break;
    	}
    	case SEND_IP_PACKET:{

    		SendPacket(1500, me->ptr_e->Packet);										//send ARP or ICMP packet on ENC28 chip
    	break;
    	}
    	case SEND_TCP_PACKET:{

    		SendPacket(1500, me->ptr_e->Packet);										//send TCP packet on ENC28 chip
    	break;
    	}
    }
}

void ETHERNET_ctor(ETHERNET * const me) {
    Active_ctor(&me->super, (DispatchHandler)&ETHERNET_dispatch);
}

static StackType_t ETHERNET_stack[250]; //task stack
static Event *ETHERNET_queue[10];
static ETHERNET AO_ETHERNET = {.ISR1_check=0};
Active *ETHERNET_ptr = &AO_ETHERNET.super;

//--------------------------------------------------------------------------------------------- IP LAYER ACTIVE OBJECT

static void IP_LAYER_dispatch(IP_LAYER * const me, Event * const e) {

	static PacketEvent IP_Packet_Event = {.Signal.sig=SEND_IP_PACKET};						//event signal with parameter Packet[] (send IP packet back to ETHERNET)
	static Event IP_Event = {RECEIVING_TCP_PACKET};											//event signal used by the IP layer	   (pass TCP packet to TCP SERVER)
	me->ptr_e = (PacketEvent*)&e->sig;														//assign a pointer to PacketEvent struct to current event e

    switch (e->sig) {
    	case INIT_SIG:{

			ARP_Request(IP_Packet_Event.Packet);											//creating ARP request to the local router for obtaining an IP number
			Active_post(ETHERNET_ptr, (Event *)&IP_Packet_Event.Signal.sig);				//post the created event to ETHERNET queue(for sending)
    		break;
    	}
    	case RECEIVING_IP_PACKET:{

    		if(me->ptr_e->Packet[18]==0x08 && me->ptr_e->Packet[19]==0x06 &&  				//is the packet an ARP request-
    				me->ptr_e->Packet[21]==0x01 &&  me->ptr_e->Packet[47]==0x1e){			//-with the last IP number byte .30?

    			ARP_Reply(IP_Packet_Event.Packet);											//build an ARP reply into event Packet
    			Active_post(ETHERNET_ptr, (Event *)&IP_Packet_Event.Signal.sig);			//post the created event back to ETHERNET queue(for sending)
    		}

    		else if (me->ptr_e->Packet[18]==0x08 && me->ptr_e->Packet[19]==0x00 &&  me->ptr_e->Packet[29]==0x01 &&  //is packet IP - ICMP ping-
    				me->ptr_e->Packet[39]==0x1e && me->ptr_e->Packet[40]==0x08){									//-with the last IP byte .30?

    			ICMP(0, 0, 0, 0, IP_Packet_Event.Packet);															//build ICMP reply into event Packet
    			Active_post(ETHERNET_ptr, (Event *)&IP_Packet_Event.Signal.sig);									//post the created event back to ETHERNET queue(for sending)
    		}

    		else if (me->ptr_e->Packet[18]==0x08 && me->ptr_e->Packet[19]==0x00 &&
    				me->ptr_e->Packet[29]==0x06 && me->ptr_e->Packet[39]==0x1e){ 									//is it an IP-TCP packet with the last IP byte .30?

            	    BaseType_t status = xQueueSend(TCP_param_queue, &me->ptr_e->Packet, (TickType_t)0);				//post first 100 bytes of the received ETHERNET packet to TCP_param_queue
            	    configASSERT(status == pdTRUE);																	//check if posted
            		Active_post(TCP_SERVER_ptr, (Event *)&IP_Event);												//Send event to TCP
    		}
    	break;
    	}
    }
}
void IP_LAYER_ctor(IP_LAYER * const me) {
    Active_ctor(&me->super, (DispatchHandler)&IP_LAYER_dispatch);
}

static StackType_t IP_LAYER_stack[350];
static Event *IP_LAYER_queue[10];
static IP_LAYER AO_IP_LAYER;
Active *IP_LAYER_ptr = &AO_IP_LAYER.super;

//--------------------------------------------------------------------------------------------- TCP SERVER ACTIVE OBJECT

static void TCP_SERVER_dispatch(TCP_SERVER * const me, Event const * const e) {

	static PacketEvent TCP_Packet_Event = {.Signal.sig=SEND_TCP_PACKET};		//event signal with parameter Packet[] (send TCP packet back to ETHERNET)
	static Event TCP_Event_1 = {.sig = INCOMING_OPTION};						//event signal used by TCP SERVER to signal APP for another parameter
	static Event TCP_Event_2 = {.sig = SEND_ANOTHER_PARAMETER};					//sending option to APP

    switch (e->sig) {
        case INIT_SIG:{
        	break;
        }
        case TIMEOUT_SIG:{
    		if(me->retransmission>=4){																		//if there we 4 previous retransmissions without ACK from client
    			TCP(me->client_port, RST, me->SEQ_number, me->ACK_number, 0, 0, TCP_Packet_Event.Packet);	//send RST and break the TCP connection
    			Active_post(ETHERNET_ptr, (Event *)&TCP_Packet_Event.Signal.sig);
    			me->socket=false; break;
    			}

        	TCP(me->client_port, PSH_ACK, me->saved_ACK_number, me->saved_SEQ_number, (uint8_t *)&(me->out_parameter), packet_data_size, TCP_Packet_Event.Packet); 	// reconstruct the TCP packet
        	Active_post(ETHERNET_ptr, (Event *)&TCP_Packet_Event.Signal.sig);								//re-send the packet
            TimeEvent_arm(&me->te, (1000 / portTICK_RATE_MS), 0U);											//arm timer for TCP retransmission in case of a lost packet (1s)
            me->retransmission++;
        break;
        }
        case RECEIVING_TCP_PACKET:{																			//incoming TCP packet

        	xQueueReceive(TCP_param_queue, &TCP_Packet_Event.Packet, (TickType_t)0);						//copy first 100 bytes of packet from queue to TCP array
        	update_packet_stats(&(me->SEQ_number), &(me->ACK_number), &(me->source_ip), &(me->source_port), &(me->in_parameter), &(me->flag), (uint8_t *)&TCP_Packet_Event.Packet, &(me->SEQ_ACK_state));

        	/* Creating a socket (3-way handshake) */
        	if(me->socket==false && me->flag==SYN && me->ACK_number==0){											//check if we don't have a socket yet and if the client is requesting one
        		update_SEQ_ACK_numbers(&(me->SEQ_number), &(me->ACK_number), (uint8_t *)&TCP_Packet_Event.Packet, &(me->flag), &(me->SEQ_ACK_state));
            	TCP(me->source_port, SYN_ACK, me->SEQ_number, me->ACK_number, 0, 0, TCP_Packet_Event.Packet); 		// send back SYN, ACK
            	Active_post(ETHERNET_ptr, (Event *)&TCP_Packet_Event.Signal.sig);
            	me->socket_syn=true;																				//socket synchronized
        	}

        	else if (me->socket_syn==true && me->socket==false && me->flag==ACK && me->ACK_number==1){		//else if we synchronized  the socket previously and the client is ACK it

        		me->socket_syn=false;					//reset the syn back to false
        		me->client_ip =	me->source_ip;			//set the client ip
        		me->client_port = me->source_port;		//set the client port
        		me->socket=true;						//socket was created
        	}
        	/*-------------------------------------*/

        	else if (me->socket==true && me->source_ip==me->client_ip && me->source_port==me->client_port){ 			//else if the socket exists and the source ip and port are correct

        		if(me->flag==PSH_ACK){																					//if the client is sending data
            		me->in_parameter=TCP_Packet_Event.Packet[62]<<16 | TCP_Packet_Event.Packet[61]<<8 | TCP_Packet_Event.Packet[60];
            	    BaseType_t status = xQueueSend(APP_IN_queue, &me->in_parameter, (TickType_t)0);						//post 4 bytes of parameters in packet to APP_IN_queue
            	    configASSERT(status == pdTRUE);																		//check if posted
            	    Active_post(APPLICATION_ptr, (Event *)&TCP_Event_1);													//signal SENDING_OPTION to APP
            	    update_SEQ_ACK_numbers(&(me->SEQ_number), &(me->ACK_number), (uint8_t *)&TCP_Packet_Event.Packet, &(me->flag), &(me->SEQ_ACK_state));
                	TCP(me->client_port, ACK, me->SEQ_number, me->ACK_number, 0, 0, TCP_Packet_Event.Packet);			//send ACK back to ETHERNET
                	Active_post(ETHERNET_ptr, (Event *)&TCP_Packet_Event.Signal.sig);
        		}

        		if ((me->flag==ACK || me->flag==PSH_ACK) && me->SEQ_number==me->saved_ACK_number && me->ACK_number==me->saved_SEQ_number+packet_data_size){//if the client is ACKing data sent by us
        			me->retransmission=0;																									//set retransmission counter to 0
        			TimeEvent_disarm(&me->te);																								//disarm timer for retransmission
        			Active_post(APPLICATION_ptr, (Event *)&TCP_Event_2);																	//notify APP layer for a new parameter
        		}

        		if (me->flag==RST || me->flag==RST_ACK || me->flag==FIN_ACK){												//if the client is sending a RST flag or finishing the TCP connection
        			update_SEQ_ACK_numbers(&(me->SEQ_number), &(me->ACK_number), (uint8_t *)&TCP_Packet_Event.Packet, &(me->flag), &(me->SEQ_ACK_state));
        			TCP(me->client_port, FIN_ACK, me->SEQ_number, me->ACK_number, 0, 0, TCP_Packet_Event.Packet);			//send FIN_ACK back
        			TimeEvent_disarm(&me->te);
        			Active_post(ETHERNET_ptr, (Event *)&TCP_Packet_Event.Signal.sig);
            		me->socket=false;																					//delete the socket

            		me->in_parameter = 0;																				//in_parameter = 0 to stop the APPLICATION from sending data
            	    BaseType_t status = xQueueSend(APP_IN_queue, &me->in_parameter, (TickType_t)0);						//post 4 bytes of parameters in packet to APP_IN_queue
            	    configASSERT(status == pdTRUE);																		//check if posted
            	    Active_post(APPLICATION_ptr, (Event *)&TCP_Event_1);												//signal SENDING_OPTION to APP
        		}
        	}
        break;
        }
        case RECEIVING_APP_PARAMETER:{
        	xQueueReceive(APP_OUT_queue, &(me->out_parameter), (TickType_t)0);																			//read queue (2 bytes)
        	update_SEQ_ACK_numbers(&(me->SEQ_number), &(me->ACK_number), (uint8_t *)&TCP_Packet_Event.Packet, &(me->flag), &(me->SEQ_ACK_state));
        	TCP(me->client_port, PSH_ACK, me->SEQ_number, me->ACK_number, (uint8_t *)&(me->out_parameter), packet_data_size, TCP_Packet_Event.Packet); 	// construct TCP packet
        	Active_post(ETHERNET_ptr, (Event *)&TCP_Packet_Event.Signal.sig);																			//send the created packet
            TimeEvent_arm(&me->te, (6000 / portTICK_RATE_MS), 0U);																						//arm timer for TCP retransmission in case of a lost packet (6s)
            me->saved_SEQ_number = me->SEQ_number;
            me->saved_ACK_number = me->ACK_number;
        break;
        }
    }
}

void TCP_SERVER_ctor(TCP_SERVER * const me) {
    Active_ctor(&me->super, (DispatchHandler)&TCP_SERVER_dispatch);
    TimeEvent_ctor(&me->te, TIMEOUT_SIG, &me->super);
}

static StackType_t TCP_SERVER_stack[350];
static Event *TCP_SERVER_queue[10];
static TCP_SERVER AO_TCP_SERVER={.socket_syn=false, .socket=false, .retransmission=0};
Active *TCP_SERVER_ptr = &AO_TCP_SERVER.super;

//-------------------------------------------------------------------------------

static void APPLICATION_dispatch(APPLICATION * const me, Event const * const e) {
	static Event APPLICATION_Event_1 = {.sig = BATTERY_CHECK};
	static Event APPLICATION_Event_2 = {.sig = ANGLE_CHECK};
	static Event APPLICATION_Event_3 = {.sig = MOTOR_CONTROL};
	static Event APPLICATION_Event_4 = {.sig = RECEIVING_APP_PARAMETER};

    switch (e->sig) {
    	case INIT_SIG:{
    		break;
    	}
    	case TIMEOUT_SIG:{
    		MPU6050_Read_Gyro();
    		Active_post(APPLICATION_ptr, (Event *)&APPLICATION_Event_2);	//post event to its own queue (ANGLE_CHECK)
    		break;
    	}

    	case INCOMING_OPTION:{
    		xQueueReceive(APP_IN_queue, me->option, (TickType_t)0);
    		me->parameter = me->option[1]<<8 | me->option[0];
    		if(!((me->option[2])>=0 && (me->option[2])<4 && me->parameter<=100 && me->parameter>=-100)){	//check the option parameters
    			break;
    		}
			TimeEvent_disarm(&me->te);
			yaw = 0;
    		if(me->option[2]==1){
    			Active_post(APPLICATION_ptr, (Event *)&APPLICATION_Event_1);	//post event to its own queue (BATTERY_CHECK)
    		}else if (me->option[2] == 2){
    			TimeEvent_arm(&me->te, (300 / portTICK_RATE_MS), 0U);
    			Active_post(APPLICATION_ptr, (Event *)&APPLICATION_Event_2);	//post event to its own queue (ANGLE_CHECK)
    		}else if (me->option[2] == 3){
    			Active_post(APPLICATION_ptr, (Event *)&APPLICATION_Event_3);	//post event to its own queue (MOTOR_CONTROL)
    		}
    		break;
    	}
    	case SEND_ANOTHER_PARAMETER:{											//called by TCP server for more parameters (ANGLE_CHECK)
    		if (me->option[2] == 2 ){
    			TimeEvent_arm(&me->te, (300 / portTICK_RATE_MS), 0U);
    		}
    		break;
    	}

    	case BATTERY_CHECK:{
			xQueueSend(APP_OUT_queue, adc_buf, (TickType_t)0);
			Active_post(TCP_SERVER_ptr, (Event *)&APPLICATION_Event_4);
    		break;
    	}

    	case ANGLE_CHECK:{
    			int32_t angle = yaw;
    			xQueueSend(APP_OUT_queue, &angle, (TickType_t)0);				//post the parameter to APP_OUT_queue
    			Active_post(TCP_SERVER_ptr, (Event *)&APPLICATION_Event_4);
    	    break;
    	}

    	case MOTOR_CONTROL:{

    		me->PWM=me->option[1]<<8 | me->option[0];
    		me->PWM=constrain(me->PWM, 100, -100);
    		if (me->PWM < 0){
    			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    		}else{
    			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    		}
    		me->PWM=abs(me->PWM);
    		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (me->PWM)*6);
    		break;
    	}
    }
}

void APPLICATION_ctor(APPLICATION * const me) {
    Active_ctor(&me->super, (DispatchHandler)&APPLICATION_dispatch);
    TimeEvent_ctor(&me->te, TIMEOUT_SIG, &me->super);
}

static StackType_t APPLICATION_stack[350];
static Event *APPLICATION_queue[10];
static APPLICATION AO_APPLICATION = {};
Active *APPLICATION_ptr = &AO_APPLICATION.super;

//------------------------------------------------------------------------------

uint8_t start(){

	ETHERNET_ctor(&AO_ETHERNET);
	Active_start(ETHERNET_ptr,
               	 2U,
				 ETHERNET_queue,
				 sizeof(ETHERNET_queue)/sizeof(ETHERNET_queue[0]),			//number of elements in the array
				 ETHERNET_stack,
				 sizeof(ETHERNET_stack),
				 0U);

	IP_LAYER_ctor(&AO_IP_LAYER);
	Active_start(IP_LAYER_ptr,
               	 3U,
				 IP_LAYER_queue,
				 sizeof(IP_LAYER_queue)/sizeof(IP_LAYER_queue[0]),
				 IP_LAYER_stack,
				 sizeof(IP_LAYER_stack),
				 0U);

	TCP_SERVER_ctor(&AO_TCP_SERVER);
	Active_start(TCP_SERVER_ptr,
               	 1U,
				 TCP_SERVER_queue,
				 sizeof(TCP_SERVER_queue)/sizeof(TCP_SERVER_queue[0]),
				 TCP_SERVER_stack,
				 sizeof(TCP_SERVER_stack),
				 0U);

	APPLICATION_ctor(&AO_APPLICATION);
	Active_start(APPLICATION_ptr,
               	 4U,
				 APPLICATION_queue,
				 sizeof(APPLICATION_queue)/sizeof(APPLICATION_queue[0]),
				 APPLICATION_stack,
				 sizeof(APPLICATION_stack),
				 0U);

	TCP_param_queue = xQueueCreateStatic(
  			   	 sizeof(TCP_parameters_queue)/100,   							// queue length
                 100,     														// item size (bytes)
                 TCP_parameters_queue, 											// queue storage
                 &TCP_param_queue_cb);      									// queue control block
	configASSERT(TCP_param_queue); 												// queue must be created

	APP_IN_queue = xQueueCreateStatic(
  			   	 sizeof(APP_INPUT_queue)/4,    									// queue length
                 4,     														// item size (bytes)
                 APP_INPUT_queue, 												// queue storage
                 &APP_IN_queue_cb);      										// queue control block
	configASSERT(APP_IN_queue); 												// queue must be created

	APP_OUT_queue = xQueueCreateStatic(
  			   	 sizeof(APP_OUTPUT_queue)/4,    								// queue length
                 4,     														// item size (bytes)
                 APP_OUTPUT_queue, 												// queue storage
                 &APP_OUT_queue_cb);      										// queue control block
	configASSERT(APP_OUT_queue); 												// queue must be created

     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
     HAL_Delay(10);
     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
     HAL_Delay(1000);
     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

	 ENC20J60Init();

	 HAL_TIM_Base_Start(&htim6);
	 MPU6050_Init();
	 HAL_Delay (100);  // wait for .1 sec
	 HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 2);
	 HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	 __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);

	 vTaskStartScheduler();

	 return 0; 						//the scheduler does NOT return
}
