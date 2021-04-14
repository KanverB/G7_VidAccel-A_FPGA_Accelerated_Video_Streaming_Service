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
#define ENC_RGB_ADDR	0x008
#define ENC_CCC_ADDR	0x048
#define DEC_START_ADDR	0x050
#define DEC_DONE_ADDR	0x054
#define DEC_CCC_ADDR	0x058
#define DEC_RGB_ADDR	0x060


u32 rgb_data[16];
u32 ccc_data[2];


u32 read_reg(u32 offset) {
	u32 data = CCC_CODEC_IP_mReadReg (XPAR_CCC_CODEC_IP_0_S00_AXI_BASEADDR, offset);
	xil_printf ("Reading register: address 0x%.3x, value 0x%.8x\n", offset, data);
	return data;
}

void write_reg(u32 offset, u32 data) {
	xil_printf ("Writing register: address 0x%.3x, value 0x%.8x\n", offset, data);
	CCC_CODEC_IP_mWriteReg (XPAR_CCC_CODEC_IP_0_S00_AXI_BASEADDR, offset, data);
//	xil_printf ("Done writing\n");
}


void stop_enc() {
	write_reg(ENC_START_ADDR, 0);
}

void start_enc() {
	write_reg(ENC_START_ADDR, 1);
}

void wait_for_enc_done() {
	u32 TIMEOUT = 5;
	u32 i;
	for (i = 0; i < TIMEOUT; i++) {
		if (read_reg(ENC_DONE_ADDR) != 0) {
			xil_printf("Found enc done\n");
			break;
		}
	}
	xil_printf("Count: %u\n", i);
}

// y, x - 0 to 3
// r, g, b - 8-bit data
void write_enc_rgb(int y, int x, u8 r, u8 g, u8 b) {
	int i_pixel = (y * 4) + x;
	int addr = ENC_RGB_ADDR + (i_pixel * 4);
	u32 data =
			(((u32) r) << 16) |
			(((u32) g) <<  8) |
			(((u32) b) <<  0);
	write_reg(addr, data);
}

// i - 0 or 1
u32 read_enc_ccc(int i) {
	int addr = ENC_CCC_ADDR + (i * 4);
	u32 data = read_reg(addr);
	return data;
}


void stop_dec() {
	write_reg(DEC_START_ADDR, 0);
}

void start_dec() {
	write_reg(DEC_START_ADDR, 1);
}

void wait_for_dec_done() {
	u32 TIMEOUT = 5;
	u32 i;
	for (i = 0; i < TIMEOUT; i++) {
		if (read_reg(DEC_DONE_ADDR) != 0) {
			xil_printf("Found dec done\n");
			break;
		}
	}
	xil_printf("Count: %u\n", i);
}

// i - 0 or 1
// data - 32-bit data
void write_dec_ccc(int i, u32 data) {
	int addr = DEC_CCC_ADDR + (i * 4);
	write_reg(addr, data);
}

// y, x - 0 to 3
// r, g, b - 8-bit data, these are like 3 return values so need to pass pointers in
void read_dec_rgb(int y, int x, u8* r, u8* g, u8* b) {
	int i_pixel = (y * 4) + x;
	int addr = DEC_RGB_ADDR + (i_pixel * 4);
	u32 data = read_reg(addr);
	*r = (data >> 16) & 0xFF;
	*g = (data >>  8) & 0xFF;
	*b = (data >>  0) & 0xFF;
}




int main()
{
    init_platform();
    print("Hello World\n\r");


    const int ITERATIONS = 3;

    for (int iter = 0; iter < ITERATIONS; iter++) {
    	xil_printf("Starting iteration %d\n", iter);

		wait_for_enc_done();
		stop_enc();
		wait_for_enc_done();

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int i = (y * 4) + x;
				write_enc_rgb(y, x, i * 4, 32, (15 - i) * 4);
			}
		}

		wait_for_enc_done();
		start_enc();
		wait_for_enc_done();

		for (int i = 0; i < 2; i++) {
			ccc_data[i] = read_enc_ccc(i);
		}




		wait_for_dec_done();
		stop_dec();
		wait_for_dec_done();

		for (int i = 0; i < 2; i++) {
			write_dec_ccc(i, ccc_data[i]);
		}

		wait_for_dec_done();
		start_dec();
		wait_for_dec_done();

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				u8 r, g, b;
				read_dec_rgb(y, x, &r, &g, &b);
			}
		}




		wait_for_enc_done();
		wait_for_dec_done();

		xil_printf("Done iteration %d\n", iter);
    }

	xil_printf("Done program\n");
    cleanup_platform();
    return 0;
}
