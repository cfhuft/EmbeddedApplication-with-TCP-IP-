/*
 * ENC28J60_init.c
 *
 *  Created on: Jul 30, 2021
 *      Author: Samo Novak
 */

#include <ENC28J60_init.h>

static uint8_t currentBank = 0;
static uint16_t nextReadPtr = 0;


void ReadReg(uint8_t opcode, uint8_t addr, uint8_t *buffer){

	uint8_t addrMask=0x1F;								//0011111 addr mask - 00 for bank, 11111 for 5 bit registers
	uint8_t command = opcode|(addr&addrMask);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET); 				//CS to 0 to activate the chip on pin B5
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){} 			//is SPI ready?
	HAL_SPI_Transmit(&hspi1, &command, 1, 100);					//Transmit command to chip
	if(addr & 0x80)
		{
			while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
			HAL_SPI_Receive(&hspi1, buffer, 1, 100);
		}
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
	HAL_SPI_Receive(&hspi1, buffer, 1, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);				//CS to 1 to deactivate SPI for the chip
}

void WriteReg(uint8_t opcode, uint8_t addr, uint8_t write){

	uint8_t addrMask=0x1F; 								//0011111 addr mask - 00 for bank, 11111 for 5 bit registers
	uint8_t command[2];
											//command[0] = (opcode<<5)|addr;
	command[0] = opcode|(addr&addrMask);
	command[1] = write;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
	HAL_SPI_Transmit(&hspi1, command, 2, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

void SetBank(uint8_t addr){

	uint8_t bankMask=0x60; 								//1100000 bank mask - 11 for bank, 00000 for 5 bit registers

	if (currentBank != (addr&bankMask)){
		WriteReg(BIT_FIELD_CLR, ECON1, ECON1_BSEL0|ECON1_BSEL1);
		currentBank = addr&bankMask;
		WriteReg(BIT_FIELD_SET, ECON1, currentBank>>5);
	}
}

void ReadControlRegister8bit(uint8_t addr, uint8_t *buffer){

	SetBank(addr);
	ReadReg(READ_CTRL_REG, addr, buffer);
}

void WriteControlRegister8bit(uint8_t addr, uint8_t write){

	SetBank(addr);
	WriteReg(WRITE_CTRL_REG, addr, write);
}

void ReadControlRegister16bit(uint8_t addr, uint8_t *buffer){

	SetBank(addr);
	ReadReg(READ_CTRL_REG, addr, &buffer[0]);
	ReadReg(READ_CTRL_REG, addr+1, &buffer[1]);
}

void WriteControlRegister16bit(uint8_t addr, uint16_t write){

	SetBank(addr);
	WriteReg(WRITE_CTRL_REG, addr, write&0xFF); 						//mask first 8 bits with 11111111
	WriteReg(WRITE_CTRL_REG, addr+1, (write>>8)&0xFF);					//mask second 8 bits

}

void WritePHY(uint8_t addr, uint16_t write){
	uint8_t status=1;
	WriteControlRegister8bit(MIREGADR, addr);
	WriteControlRegister8bit(MIWR, write);
	while (status==1){
	ReadControlRegister8bit(MISTAT, &status);
	status=status&1;
	}
}

void ENC20J60Init(){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
	WriteReg(SOFT_RESET, 0x1F, 0);								//soft reset
	HAL_Delay(100);										//wait longer for the clock to ready
	WriteControlRegister16bit(ERXST, 0x0000);						//start of RX buffer
	WriteControlRegister16bit(ERXND, 0x0FFF);						//end of RX buffer
	WriteControlRegister16bit(ETXST, 0x1005);						//start of TX buffer
	WriteControlRegister16bit(ETXND, 0x1FFF);						//end of TX buffer
	WriteControlRegister16bit(ERXWRPT, 0x0000);						//set RX currently writing location pointer
	WriteControlRegister16bit(ERXRDPTL, 0x0000);						//set RX currently reading location pointer
	WriteReg(BIT_FIELD_CLR, ECON1, 0x04);  							//Receive Disable RXEN=0 - safety before setting next registers
	WriteControlRegister8bit(ERXFCON, 0x20);						//packet filters
	WriteControlRegister8bit(MACON1, 0xF);
	WriteControlRegister8bit(MACON3, 0x32);
	WriteControlRegister8bit(MACON4, 0x40);
	WriteControlRegister16bit(MAMXFL, 1500);						//max frame 1500 bytes
	WriteControlRegister8bit(MABBIPG, 0x12);						//back to back gap
	WriteControlRegister16bit(MAIPG, 0x0C12);						//non back to back gap
												//^suggested values from chip datasheet


	WriteControlRegister8bit(MAADR6, 0x30);							//put in a random MAC address for ENC28
	WriteControlRegister8bit(MAADR5, 0x46);
	WriteControlRegister8bit(MAADR4, 0x46);
	WriteControlRegister8bit(MAADR3, 0x49);
	WriteControlRegister8bit(MAADR2, 0x43);
	WriteControlRegister8bit(MAADR1, 0x45);

	//Physical layer initilalisation:
	WritePHY(PHLCON, 0x0122);								//enable TX and RX LEDs
	WritePHY(PHCON2, 0x0100);								//disable loopback

	//Interrupts:
	WriteReg(BIT_FIELD_SET, ECON1, 0x04);  							//Receive Enable RXEN=1
	WriteReg(BIT_FIELD_SET, EIE, 0xC0);							//enable interrupts for RX - INTIE and PKTIE 11000000
	WriteReg(BIT_FIELD_SET, EIR, 0x40); 							//clear interrupt
	WriteControlRegister8bit(ECON2, 0x80);							//set AUTOINC to 1

	HAL_Delay(1000);
}

void WriteBuffer(uint16_t length, uint8_t* write){
	uint8_t command = WRITE_BUF_MEMORY;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
	HAL_SPI_Transmit(&hspi1, &command, 1, 100);
	command = 0xF; 										//control byte
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
	HAL_SPI_Transmit(&hspi1, &command, 1, 100);
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
	HAL_SPI_Transmit(&hspi1, write, length, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

void ReadBuffer(uint8_t* readbyte){
	uint8_t command = READ_BUF_MEMORY;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
	HAL_SPI_Transmit(&hspi1, &command, 1, 100);
	while(HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY){}
	HAL_SPI_Receive(&hspi1, readbyte, 1, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

void ReadPacket(uint8_t* read){
	uint8_t j = 0;
	uint16_t length = 0;
	memset(read, 0, 1500);

	ReadControlRegister8bit(EPKTCNT, &readEPKTCNT);						//How many packets is waiting to be read?

	if (readEPKTCNT>0){
		while(j < 2){									//first 2 bytes of the Packet contain adress of the next packet (ending of current packet +1)
			WriteControlRegister8bit(ERDPTL, nextReadPtr&0x00ff);			//update ERDPTL to the byte we want to read from
			WriteControlRegister8bit(ERDPTH, (nextReadPtr&0xff00)>>8);		//update ERDPTH to the byte we want to read from
			ReadBuffer(&read[j]);							//store the address to read[0] and read[1]
			j++;
			nextReadPtr++;								//increment -buffer read pointer - value for every byte
			if (nextReadPtr>0x0FFF){nextReadPtr=0x0000;}				//if we come to the end of the RX buffer start over
		}
		length = read[1];								//read up to the end of the current packet in buffer
		length = (length<<8) | read[0];

		for(uint16_t i=nextReadPtr; i!=(length); i++){					//read up to the end of the current packet in buffer
			WriteControlRegister8bit(ERDPTL, nextReadPtr&0x00ff);
			WriteControlRegister8bit(ERDPTH, (nextReadPtr&0xff00)>>8);
			ReadBuffer(&read[j]);							//read 1 byte
			j++;
			nextReadPtr++;								//increment -buffer read pointer- value
			if (nextReadPtr>0x0FFF){nextReadPtr=0x0000;}
		}

		WriteControlRegister8bit(ECON2, 0xC0);						//Decrement the EPKTCNT register value by one - by accessing ECON2.PKTDEC
		WriteControlRegister8bit(ERXRDPTL, length & 0x00ff);				//increase RX read pointer to clear the space of the-
		WriteControlRegister8bit(ERXRDPTH, (length & 0xff00)>>8);			//previously read packet
		length=0;
	}
}

void SendPacket(uint16_t length, uint8_t* write)
{

	while(1){
		WriteReg(BIT_FIELD_SET, ECON1, 0x80);						//set TXRST to 1
		WriteReg(BIT_FIELD_CLR, ECON1, 0x80);						//set TXRST to 0 (1 NAND 1) (Transmit Logic Reset bit 0=normal operation)
		WriteReg(BIT_FIELD_CLR, EIR, 0x02|0x08);					//set TXIF and TXERIF to 0

		WriteControlRegister16bit(EWRPTL, 0x1004);					//set where are we going to write to the TX buffer
		WriteControlRegister16bit(ETXND, 0x1004+length);				//set transmit packet end location (transmit packet start location is always initialised to 0x1004)
		WriteReg(WRITE_BUF_MEMORY, 0, 0);
		WriteBuffer(length, write);

		WriteReg(BIT_FIELD_SET, ECON1, 0x08);						//TXRTS to 1 (Transmit Request to Send)

		uint16_t count = 0;
		uint8_t write[2];								//wait for the interrupt flags
		while (1){
			ReadControlRegister8bit(EIR, &write[0]);				//read TXIF | TXERIF -TX interrupt and TX err interrupt flag
			if((((0x08|0x02) & write[0])==0) && ++count < 1000U){}
			else break;								//if they're both 0 and count<1000 -wait -else break;
		}
		ReadControlRegister8bit(EIR, &write[1]);
		if (!(write[1] & 0x02) && count < 1000U)break;  				// if error is 0 and count was <1000 break;
		WriteReg(BIT_FIELD_CLR, ECON1, 0x08);						// else cancel previous transmission -set TXRTS to 0
		break;
	}
}


