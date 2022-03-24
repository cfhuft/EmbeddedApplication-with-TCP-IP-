/*
 * ENC28J60_init.h
 *
 *  Created on: Jul 30, 2021
 *      Author: Samo Novak
 */

#ifndef INC_ENC28J60_INIT_H_
#define INC_ENC28J60_INIT_H_

#include "MPU6050_init.h"

//Operations
#define READ_CTRL_REG 0x00
#define WRITE_CTRL_REG 0x40
#define BIT_FIELD_CLR 0xA0
#define BIT_FIELD_SET 0x80
#define SOFT_RESET 0xE0
#define WRITE_BUF_MEMORY 0x7A	 //opcode + addr
#define READ_BUF_MEMORY 0x3A	//opcode + addr

//Bank 0 - control registers
#define ERDPTL 0x00
#define ERDPTH 0x01
#define EWRPTL 0x02
#define EWRPTH 0x03
#define ETXST 0x04
#define ETXND 0x06
#define ERXST 0x08
#define ERXND 0x0A
#define ERXRDPTL 0x0C
#define ERXRDPTH 0x0D
#define ERXWRPT 0x0E
#define ESTAT 0x1D


//Bank 1 - control registers
#define ERXFCON 0x18|0x20
#define EPMMO 0x08|0x20
#define EPMCS 0x10|0x20
#define EPKTCNT 0x19|0x20

//Bank 2 - control registers
#define MACON1 0x00|0x40
#define MACON3 0x02|0x40
#define MACON4 0x03|0x40
#define MAIPG 0x06|0x40|0x80
#define MABBIPG 0x04|0x40|0x80
#define MAMXFL 0x0A|0x40|0x80
#define MIREGADR 0x14|0x40|0x80
#define MIWR 0x16|0x40|0x80
#define MICMD 0x12|0x40|0x80
#define MIRD 0x18|0x40|0x80

//Bank 3 -control registers
#define MAADR1 0x04|0x60|0x80
#define MAADR2 0x05|0x60|0x80
#define MAADR3 0x02|0x60|0x80
#define MAADR4 0x03|0x60|0x80
#define MAADR5 0x00|0x60|0x80
#define MAADR6 0x01|0x60|0x80
#define MISTAT 0x0A|0x60|0x80
#define EREVID 0x12|0x60

//BANK control + DMA + RX enable
#define ECON1 0x1F
#define ECON1_BSEL0 0x01
#define ECON1_BSEL1 0x02
#define ECON2 0x1E


//PHY layer
#define PHLCON 0x14
#define PHCON2 0x10

//interrupts (TX, RX, DMA)
#define EIE	0x1B
#define EIR	0x1C

extern SPI_HandleTypeDef hspi1;
uint8_t readEPKTCNT;													//read EPKTCNT value - packet count

void ReadREG(uint8_t opcode, uint8_t addr, uint8_t *buffer);
void WriteREG(uint8_t opcode, uint8_t addr, uint8_t write);
void SetBank(uint8_t addr);
void ReadControlRegister8bit(uint8_t addr, uint8_t *buffer);
void WriteControlRegister8bit(uint8_t addr, uint8_t write);
void ReadControlRegister16bit(uint8_t addr, uint8_t *buffer);
void WriteControlRegister16bit(uint8_t addr, uint16_t write);
void ENC20J60Init(void);
void WritePHY(uint8_t addr, uint16_t write);
void WriteBuffer(uint16_t lenght, uint8_t* write);
void ReadBuffer(uint8_t* readbyte);
void SendPacket(uint16_t lenght, uint8_t* write);
void ReadPacket(uint8_t* read);

#endif /* INC_ENC28J60_INIT_H_ */
