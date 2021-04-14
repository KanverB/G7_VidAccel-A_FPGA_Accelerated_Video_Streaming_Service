/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "SD_MYCODE.h"
#include "xil_io.h"

#define START_WRITE 1020
#define DONE_WRITE 1016
#define START_READ 1012
#define DONE_READ 1008
#define RW_ADDR 1004
#define SD_READY 1000

unsigned int sdcard = (unsigned int)XPAR_SD_MYCODE_0_S00_AXI_BASEADDR;

char block_data[512];
unsigned int sd_block_addr;

static inline void write_block(char* block, unsigned int addr);

static inline void read_block(char* block, unsigned int addr);

int main()
{
    init_platform();

    for (int i = 0; i < 512; i = i + 4)
    {
    	*((unsigned int*)(block_data+i)) = 0xAFAEADAC;
    }

    sd_block_addr = 0;

    write_block(block_data,sd_block_addr);

    read_block(block_data,sd_block_addr);

    for (int i = 0; i < 512; i = i + 4)
    {
    	*((unsigned int*)(block_data+i)) = 0xCCEEFFAA;
    }

    sd_block_addr = 3;

    write_block(block_data,sd_block_addr);

    read_block(block_data,sd_block_addr);

    cleanup_platform();
    return 0;
}


static inline void write_block(char* block, unsigned int addr)
{
    //do a write of block addr
	//write the data in block into regs to send to sd card
    for (int i = 0; i < 512; i = i + 4)
    {
    	SD_MYCODE_mWriteReg(sdcard,i,*((unsigned int*)(block+i)));
    }

    SD_MYCODE_mWriteReg(sdcard,RW_ADDR,addr);

    while (SD_MYCODE_mReadReg(sdcard,SD_READY) != 0x1)
    {
    	xil_printf("SD Card ready? %d.\n\r",SD_MYCODE_mReadReg(sdcard,SD_READY));
    }

    SD_MYCODE_mWriteReg(sdcard,START_WRITE,1);

    while ( SD_MYCODE_mReadReg(sdcard,DONE_WRITE) != 0x1)
    {
    	xil_printf("SD Card write done ? %d.\n\r",SD_MYCODE_mReadReg(sdcard,DONE_WRITE));
    }

    SD_MYCODE_mWriteReg(sdcard,DONE_WRITE,0);

    xil_printf("Did a block write according to sd card controller.\n\r");
}

static inline void read_block(char* block, unsigned int addr)
{
    //do a read of block addr
	SD_MYCODE_mWriteReg(sdcard,RW_ADDR,addr);

    while ( SD_MYCODE_mReadReg(sdcard,SD_READY) != 0x1)
    {
    	xil_printf("SD Card ready? %d.\n\r",SD_MYCODE_mReadReg(sdcard,SD_READY));
    }

    SD_MYCODE_mWriteReg(sdcard,START_READ,1);

    while ( SD_MYCODE_mReadReg(sdcard,DONE_READ) != 0x1)
    {
    	xil_printf("SD Card read done ? %d.\n\r",SD_MYCODE_mReadReg(sdcard,DONE_READ));
    }

    SD_MYCODE_mWriteReg(sdcard,DONE_READ,0);

    xil_printf("Did a block read according to sd card controller.\n\r");

    //store the read out data in block
    for (int i = 0; i < 512; i = i + 4)
    {
    	*((unsigned int*)(block+i)) = SD_MYCODE_mReadReg(sdcard,i);
    	xil_printf("Block data %x.\n\r",*((unsigned int*)(block+i)));
    }
}
