/*
 * AO_shared_data.c
 *
 *  Created on: Jul 30, 2021
 *      Author: Samo Novak
 */

#include <TCP_IP_headers.h>

void Ethernet_Header_Setup(uint8_t* header, uint8_t protocol){
	header[0]=0xff; header[1]=0xff; header[2]=0xff; header[3]=0xff; header[4]=0xff; header[5]=0xff;		//target HW address
	header[6]=0x30; header[7]=0x46; header[8]=0x46; header[9]=0x49; header[10]=0x43; header[11]=0x45;	//sender HW address
	header[12]=0x08; header[13]=protocol;									//protocol
}

void ARP_Header_Setup(uint8_t* header, uint8_t opcode){
	header[0]=0x00; header[1]=0x01; 									//HW type - Ethernet
	header[2]=0x08; header[3]=0x00; 									//protocol type
	header[4]=0x06; 											//HW length
	header[5]=0x04;												//protocol length
	header[6]=0x00; header[7]=opcode; 									//operational request/reply
	header[8]=0x30; header[9]=0x46; header[10]=0x46; header[11]=0x49; header[12]=0x43; header[13]=0x45;	//sender HW address
	header[14]=0xC0; header[15]=0xA8; header[16]=0x00; header[17]=source_ip_address;			//sender IP address
	header[18]=0x00; header[19]=0x00; header[20]=0x00; header[21]=0x00; header[22]=0x00; header[23]=0x00;	//target HW address
	header[24]=0xC0; header[25]=0xA8; header[26]=0x00; header[27]=0x01;					//target IP address
}

void IP_Header_Setup(uint8_t* header, uint8_t protocol, uint32_t destination, uint16_t add_Length){
	add_Length=add_Length+20;

	header[0]=0x45;													//version+length
	header[1]=0x00; 												//type of service
	header[2]=add_Length>>8; header[3]=add_Length; 									//total length
	header[4]=0x00; header[5]=0x01;											//ID
	header[6]=0x40; header[7]=0x00; 										//flags + fragment offset
	header[8]=0x20;													//TTL
	header[9]=protocol;												//Protocol
	header[10]=0x00; header[11]=0x00;										//header checksum
	header[12]=0xC0; header[13]=0xA8; header[14]=0x00; header[15]=source_ip_address;				//source address
	header[16]=destination>>24; header[17]=destination>>16; header[18]=destination>>8; header[19]=destination;	//destination address
}

void ICMP_Header_Setup(uint8_t* header, uint8_t type, uint8_t code){
	header[0]=type;													//type
	header[1]=code; 												//code
	header[2]=0x00; header[3]=0x00; 										//checksum
	header[4]=0x00; header[5]=0x00;											//content
	header[6]=0x00; header[7]=0x00; 										//content
}

void TCP_Header_Setup(uint8_t* header, uint16_t sourcePort, uint16_t destinationPort, uint8_t TCP_flags, uint32_t SEQ_number, uint32_t ACK_number){
	header[0]=sourcePort>>8; header[1]=sourcePort; 								//source port
	header[2]=destinationPort>>8; header[3]=destinationPort;						//destination port
	header[4]=SEQ_number>>24; header[5]=SEQ_number>>16; header[6]=SEQ_number>>8; header[7]=SEQ_number;  	//sequence number
	header[8]=ACK_number>>24; header[9]=ACK_number>>16; header[10]=ACK_number>>8; header[11]=ACK_number;	//ACK number
	header[12]=0x80;											//header length in 32-bit words + 0000 for part of reserved bits
	header[13]=TCP_flags;											//flags
	header[14]=0x00; header[15]=packet_data_size;								//window size = 4B
	header[16]=0x00; header[17]=0x00;									//checksum
	header[18]=0x00; header[19]=0x00; 									//urgent pointer
	header[20]=0x00; header[21]=0x00; header[22]=0x00; header[23]=0x00; header[24]=0x00; header[25]=0x00;	//options - 12 bytes
	header[26]=0x00; header[27]=0x00; header[28]=0x00; header[29]=0x00; header[30]=0x00; header[31]=0x00;
}

void IP_Checksum_value(uint8_t* header){

	uint16_t bits=0;
	uint32_t sum=0;
	uint16_t checksum=0;

	for(int i=0; i<=19; i+=2){
		if(i==10)continue;
		bits = ((header[i]<<8)| (header[i+1]));
		sum += bits;
		checksum=~(sum+(sum>>16));
	}
	header[10]=checksum>>8;
	header[11]= checksum;
}

void ICMP_Checksum_value(uint8_t* header, uint8_t *append_Data, uint16_t appended_data_length){

	uint16_t bits=0;
	uint32_t sum=0;
	uint16_t checksum=0;

	for(int i=0; i<=7; i+=2){
		if(i==2)continue;
		bits = ((header[i]<<8)| (header[i+1]));
		sum += bits;
	}

	if(append_Data!=0){							//if there is some appended data
		for(int i=0; i<=appended_data_length; i+=2){			//add it to checksum
			bits = ((append_Data[i]<<8)| append_Data[i+1]);
			sum += bits;
		}
	}

	checksum=~(sum+(sum>>16)); 						//sum of bits over 16 bit to the start of checksum
	header[2]= checksum>>8;							//write the checksum in header
	header[3]= checksum;
}

void TCP_Checksum_value(uint8_t* header, uint8_t* IP_header, uint16_t TCP_length){

	uint16_t bits=0;
	uint32_t sum=0;
	uint16_t checksum=0;

	for(int i=0; i<=(TCP_length-1); i+=2){
		if(i==16)continue;
		bits = ((header[i]<<8)| (header[i+1]));
		sum += bits;
	}
	//add IP pseudo header bits to TCP checksum
	for(int i=12; i<=19; i+=2){						//source+destination IP
		bits = ((IP_header[i]<<8)| (IP_header[i+1]));
		sum += bits;
	}
	bits = IP_header[9];							//Protocol type
	sum += bits;
	bits = TCP_length;							//TCP header + data length
	sum += bits;

	checksum=~(sum+(sum>>16));
	header[16]=checksum>>8;
	header[17]= checksum;
}

void ARP_Request(uint8_t *Packet){
	//Ethernet-14 bytes;
	//ARP-28 bytes;
	memset(Packet, 0, 1500);

	Ethernet_Header_Setup(&Packet[0], 0x06);				//ARP
	ARP_Header_Setup(&Packet[14], 0x01);					//ARP Request
}

void ARP_Reply(uint8_t *Packet){
	//Ethernet-14 bytes;
	//ARP-28 bytes;
	memset(Packet, 0, 1500);

	Ethernet_Header_Setup(&Packet[0], 0x06);				//ARP
	ARP_Header_Setup(&Packet[14], 0x02);					//ARP Reply
}

void ICMP(uint8_t type, uint8_t code, uint8_t *append_Data, uint16_t appended_data_length, uint8_t *Packet){
	//Ethernet-14 bytes;
	//IP-20 bytes;
	//ICMP-8 bytes;
	memset(Packet, 0, 1500);

	Ethernet_Header_Setup(&Packet[0], 0x00);						//IP
	IP_Header_Setup(&Packet[14], 0x01, destination_ip_address, 8+appended_data_length);	//8 bytes for ICMP + appended ICMP data
	IP_Checksum_value(&Packet[14]);
	ICMP_Header_Setup(&Packet[14+20], type, code);
	ICMP_Checksum_value(&Packet[14+20], append_Data, appended_data_length);

	memcpy(&Packet[14+20+8], append_Data, appended_data_length);
}

void TCP(uint16_t TCP_destination_port, uint8_t TCP_flags, uint32_t SEQ_number, uint32_t ACK_number,
								uint8_t *append_Data, uint16_t appended_data_length, uint8_t *Packet){
	//Ethernet-14 bytes;
	//IP-20 bytes;
	//TCP-32 bytes;
	memset(Packet, 0, 1500);

	Ethernet_Header_Setup(&Packet[0], 0x00);						//IP
	IP_Header_Setup(&Packet[14], 0x06, destination_ip_address, 32+appended_data_length);	//32 bytes for TCP + appended TCP data
	IP_Checksum_value(&Packet[14]);
	TCP_Header_Setup(&Packet[14+20], TCP_source_port, TCP_destination_port, TCP_flags, SEQ_number, ACK_number);
	memcpy(&Packet[14+20+32], append_Data, appended_data_length);
	TCP_Checksum_value(&Packet[14+20], &Packet[14], 32+appended_data_length);
}

//update after receiving TCP packet
void update_packet_stats(uint32_t *SEQ_number, uint32_t *ACK_number, uint32_t *source_ip, uint16_t *source_port, uint32_t *in_parameter, uint8_t *flag, uint8_t *Packet, bool *SEQ_ACK_state){
	*SEQ_number = (Packet[44]<<24)|(Packet[45]<<16)|(Packet[46]<<8)|(Packet[47]); //update SEQ number
	*ACK_number = (Packet[48]<<24)|(Packet[49]<<16)|(Packet[50]<<8)|(Packet[51]); //update ACK number

	*source_ip = (Packet[36]<<24)|(Packet[37]<<16)|(Packet[38]<<8)|(Packet[39]);
	*source_port = (Packet[40]<<8)|(Packet[41]);
	*in_parameter = (Packet[60]<<8)|(Packet[61]);
	*flag = Packet[53];
	*SEQ_ACK_state = false;					//used to correct the ACK and SEQ numbers in case of sending data right after confirming the received packet
}

//update before sending TCP packet
void update_SEQ_ACK_numbers(uint32_t *SEQ_number, uint32_t *ACK_number, uint8_t *Packet, uint8_t *flag, bool *SEQ_ACK_state){

	if(*SEQ_ACK_state == false){
		uint32_t SEQ_save;
		uint32_t ACK_save;
		uint8_t TCP_header_len = ((Packet[52])>>4)*4;
		uint8_t IP_header_len = 20;

		SEQ_save = *SEQ_number;
		ACK_save = *ACK_number;
		*SEQ_number = ACK_save;										//SEQ is previous ACK
		*ACK_number = ((Packet[22]<<8) | Packet[23]) - (TCP_header_len+IP_header_len) + SEQ_save;	//ACK is previous SEQ + packet length - header lengths

		if (*flag==SYN || *flag==RST || *flag==RST_ACK || *flag==FIN_ACK){
			*ACK_number +=1;
		}
		*SEQ_ACK_state = true;			//used to correct the ACK and SEQ numbers in case of sending data right after confirming the received packet
	}
}

