// Taken partially from
// https://github.com/weisskopfjens/esp32voipdoorbell/tree/master

/*
* ========================================
 * [] File Name : RTPPacket.cpp
 *
 * [] Creation Date : 05-06-2017
 *
 * [] Created By : Parham Alvani (parham.alvani@gmail.com)
 * =======================================
*/

/*
 * Copyright (c) 2017 Parham Alvani.
*/

/* Bodged to work with the PI TeleCow Derek Woodroffe */

#include <string.h>
#include <stdio.h>
#include "RTPPacket.h"

uint16_t RTPPacket_payloadLength;
uint8_t RTPPacket_payloadType;
uint32_t RTPPacket_timestamp;
uint32_t RTPPacket_ssrc;
uint16_t RTPPacket_sequenceNumber;
const uint8_t *RTPPacket_payload;
uint8_t RTP_buff[2048];


void RTPPacket_init(){
   RTPPacket_sequenceNumber=0;
   RTPPacket_timestamp = 0;
   RTPPacket_payloadType =0;
   RTPPacket_ssrc=0;
}

uint16_t RTPPacket_RTPPacket(const uint8_t *payload, uint16_t payloadLength){
    RTPPacket_payload = payload;
    RTPPacket_payloadLength = payloadLength;
    
    // incrementals
    RTPPacket_sequenceNumber++;
    RTPPacket_timestamp+=125*160; //160 samples 8khz 125 uS granularity. 
    int r= RTPPacket_serialize(RTP_buff);
    return r;    
}

uint16_t RTPPacket_serialize(uint8_t *buff){
    /* buff[0] = (V << 6 | P << 5 | X << 4 | CC) */
    buff[0] = (2 << 6 | 0 << 5 | 0 << 4 | 0);
    /* buff[1] = (M << 7 | PT) */
    buff[1] = (0 << 7 | RTPPacket_payloadType);
    /* buff[2, 3] = SN */
    buff[2] = RTPPacket_sequenceNumber >> 8;
    buff[3] = RTPPacket_sequenceNumber;
    /* buff[4, 5, 6, 7] = TS */
    buff[4] = RTPPacket_timestamp >> 24;
    buff[5] = RTPPacket_timestamp >> 16;
    buff[6] = RTPPacket_timestamp >> 8;
    buff[7] = RTPPacket_timestamp;
    /* buff[8, 9, 10, 11] = SSRC */
    buff[8] = RTPPacket_ssrc >> 24;
    buff[9] = RTPPacket_ssrc >> 16;
    buff[10] = RTPPacket_ssrc >> 8;
    buff[11] = RTPPacket_ssrc;
    
    int i = RTPPacket_payloadLength;
    
    if (i == 0) {
    	while (RTPPacket_payload[i] != 0) {
	    buff[12 + i] = RTPPacket_payload[i];
	    i++;
	}
	buff[12 + i] = 0;
    } else {
        int j;
        for (j = 0; j < i; j++) {
            buff[12 + j] = RTPPacket_payload[j];
        }
        buff[12 + j] = 0;
    }
    return i + 12;
}

uint16_t RTPPacket_deserialize(const uint8_t *buff, uint16_t length){
	/* buff[0] = (V << 6 | P << 5 | X << 4 | CC) */
	if ((buff[0] & 0xC0) >> 6 != 2) {
		return 0;
	}
	/* buff[1] = (M << 7 | PT) */
	RTPPacket_payloadType = (buff[1] & 0x7F);
	/* buff[2, 3] = SN */
	RTPPacket_sequenceNumber = buff[2] << 8 | buff[3];
	/* buff[4, 5, 6, 7] = TS */
	RTPPacket_timestamp = buff[4] << 24 | buff[5] << 16 | buff[6] << 8 | buff[7];
	/* buff[8, 9, 10, 11] = SSRC */
	RTPPacket_ssrc = buff[8] << 24 | buff[9] << 16 | buff[10] << 8 | buff[11];
	RTPPacket_payloadLength = length - 12; // 12byte header
	RTPPacket_payload = buff+12;
	return length-12;
}

void RTPPacket_RTPDisplay(void){
   printf("RTP: Length:%i Type:%i TimeStamp:%i, SSRC:%i, Sequence:%i \n",
       RTPPacket_payloadLength,RTPPacket_payloadType,RTPPacket_timestamp,RTPPacket_ssrc,RTPPacket_sequenceNumber);
}

