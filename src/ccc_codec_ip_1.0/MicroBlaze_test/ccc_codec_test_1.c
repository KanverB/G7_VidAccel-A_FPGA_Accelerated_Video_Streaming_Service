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

/*
 * CCC codec test program
 *
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

#include "xil_io.h"
#include "ccc_codec_ip.h"

#define ENC_START_ADDR	0x000
#define ENC_DONE_ADDR	0x004
#define DEC_START_ADDR	0x008
#define DEC_DONE_ADDR	0x00C
#define RGB_ADDR		0x010
#define CCC_ADDR		0xA10


u32 read_reg(u32 offset) {
	u32 data = CCC_CODEC_IP_mReadReg (XPAR_CCC_CODEC_IP_0_S00_AXI_BASEADDR, offset);
	xil_printf ("Reading register value at address 0x%.3x: 0x%.8x\n", offset, data);
	return data;
}

void write_reg(u32 offset, u32 data) {
	xil_printf ("Writing register value at address 0x%.3x: 0x%.8x\n", offset, data);
	CCC_CODEC_IP_mWriteReg (XPAR_CCC_CODEC_IP_0_S00_AXI_BASEADDR, offset, data);
	xil_printf ("Done writing\n");
}

void wait_for_enc_done() {
	u32 TIMEOUT = 100;
	u32 i;
	for (i = 0; i < TIMEOUT; i++) {
		if (read_reg(ENC_DONE_ADDR) != 0) {
			xil_printf("Found enc done\n");
			break;
		}
	}
	xil_printf("Count: %u\n", i);
}

void wait_for_dec_done() {
	u32 TIMEOUT = 100;
	u32 i;
	for (i = 0; i < TIMEOUT; i++) {
		if (read_reg(DEC_DONE_ADDR) != 0) {
			xil_printf("Found dec done\n");
			break;
		}
	}
	xil_printf("Count: %u\n", i);
}


int main()
{
    init_platform();
    print("Hello World\n\r");

//    CCC_CODEC_IP_Reg_SelfTest();

    read_reg(0);
    write_reg(0, 0);
    read_reg(ENC_START_ADDR);
    read_reg(ENC_DONE_ADDR);
    read_reg(DEC_START_ADDR);
    read_reg(DEC_DONE_ADDR);
    read_reg(RGB_ADDR);
	read_reg(CCC_ADDR);

	wait_for_enc_done();
	write_reg(ENC_START_ADDR, 1);
	wait_for_enc_done();

	wait_for_dec_done();
	write_reg(DEC_START_ADDR, 1);
	wait_for_dec_done();

    cleanup_platform();
    return 0;
}
