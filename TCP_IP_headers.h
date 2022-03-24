/*
 * AO_shared_data.h
 *
 *  Created on: Jul 30, 2021
 *      Author: Samo Novak
 */

#ifndef INC_TCP_IP_HEADERS_H_
#define INC_TCP_IP_HEADERS_H_

#include "ENC28J60_init.h"
#include <stdbool.h>

static const uint32_t destination_ip_address = 3232235636;	//192.168.0.116
static const uint8_t source_ip_address = 0x1e;			// .30 (192.168.0.30)
static const uint16_t TCP_source_port = 54000;
static const uint8_t packet_data_size = 4;			//incoming and outgoing data in packets will be 4 Bytes


/*TCP FLAGS*/
#define ACK 0x10
#define SYN_ACK 0x12
#define SYN 0x02
#define PSH_ACK 0x18
#define RST_ACK 0x14
#define RST 0x04
#define FIN_ACK 0x11
#define PSH 0x8


void Ethernet_Header_Setup(uint8_t* header, uint8_t protocol);
void ARP_Header_Setup(uint8_t* header, uint8_t opcode);
void IP_Header_Setup(uint8_t* header, uint8_t protocol, uint32_t destination, uint16_t add_Length);
void ICMP_Header_Setup(uint8_t* header, uint8_t type, uint8_t code);
void TCP_Header_Setup(uint8_t* header, uint16_t sourcePort, uint16_t destinationPort, uint8_t TCP_flags, uint32_t SEQ_number, uint32_t ACK_number);
void IP_Checksum_value(uint8_t* header);
void ICMP_Checksum_value(uint8_t* header, uint8_t *append_Data, uint16_t appended_data_length);
void TCP_Checksum_value(uint8_t* header, uint8_t* IP_header, uint16_t TCP_length);
void ARP_Request(uint8_t *Packet);
void ARP_Reply(uint8_t *Packet);
void ICMP(uint8_t type, uint8_t code, uint8_t *append_Data, uint16_t appended_data_length, uint8_t *Packet);
void TCP(uint16_t TCP_destination_port, uint8_t TCP_flags, uint32_t SEQ_number, uint32_t ACK_number,
				uint8_t *append_Data, uint16_t appended_data_length, uint8_t *Packet);

void update_packet_stats(uint32_t *SEQ_number, uint32_t *ACK_number, uint32_t *source_ip, uint16_t *source_port, uint32_t *in_parameter, uint8_t *flag, uint8_t *Packet, bool *SEQ_ACK_state);
void update_SEQ_ACK_numbers(uint32_t *SEQ_number, uint32_t *ACK_number, uint8_t * Packet, uint8_t *flag, bool *SEQ_ACK_state);

#endif /* INC_TCP_IP_HEADERS_H_ */
