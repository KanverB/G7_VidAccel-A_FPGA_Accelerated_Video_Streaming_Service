/*
 * Copyright (C) 2009 - 2018 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>

#include "xparameters.h"

#include "netif/xadapter.h"

#include "lwip/etharp.h"

#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif

#include "lwip/tcp.h"
#include "xil_cache.h"

#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "lwip/etharp.h"
#if LWIP_IPV6==1
#include "lwip/ip.h"
#else
#if LWIP_DHCP==1
#include "lwip/dhcp.h"
#endif
#endif

//switch between client and server
#define CLIENT 0

//SD Card headers and defines
#include "SD_MYCODE.h"
#include "xil_io.h"

#define START_WRITE 1020
#define DONE_WRITE 1016
#define START_READ 1012
#define DONE_READ 1008
#define RW_ADDR 1004
#define SD_READY 1000

unsigned int sdcard = (unsigned int)XPAR_SD_MYCODE_0_S00_AXI_BASEADDR;

static inline void write_block(char* block, unsigned int addr);

static inline void read_block(char* block, unsigned int addr);

void write_frames_to_sd();

void read_frames_from_sd();

//AXI Timer headers and defines
#include "xtmrctr.h"

#define TIMER_COUNTER_0	 0

XTmrCtr TimerCounter; /* The instance of the Tmrctr Device */

u32 time1;

u32 time2;

//uart headers and defines
#include "xuartlite.h"

#define UARTLITE_DEVICE_ID	  XPAR_UARTLITE_0_DEVICE_ID

static void UartLiteSendHandler(void *CallBackRef, unsigned int EventData);

static void UartLiteRecvHandler(void *CallBackRef, unsigned int EventData);

XUartLite UartLiteInst;  /* The instance of the UartLite Device */

/* defined by each RAW mode application */
void print_app_header();
int start_application();
int transfer_data();
void tcp_fasttmr(void);
void tcp_slowtmr(void);

/* missing declaration in lwIP */
void lwip_init();

//TCP Network Params
#define DEST_IP4_ADDR  "1.1.22.1"
#define DEST_IP6_ADDR "fe80::6600:6aff:fe71:fde6"
#define DEST_PORT 50000

#define SRC_PORT 50001

int setup_client_conn();
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_client_err(void *arg, err_t err);
static void tcp_client_close(struct tcp_pcb *pcb);

#if LWIP_IPV6==0
#if LWIP_DHCP==1
extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);
#endif
#endif

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
static struct netif server_netif;
struct netif *echo_netif;
struct netif *app_netif;
static struct tcp_pcb *c_pcb;
char is_connected;

//160x120x3+15 header (bytes)
#if CLIENT == 1
	#define FRAME_SIZE 9615
#else
//160x120x3/6+15 header (bytes)
	#define FRAME_SIZE 57615
#endif
//large static global array to hold frame data
#define FRAME_SIZE_FULL 57615
//NUM_FRAMES should be 137
#define NUM_FRAMES 137
unsigned char VIDEO[FRAME_SIZE_FULL * NUM_FRAMES];
int frameNum;

//large static global array to hold encoded frame data
//we can compress by a factor of 6
//160x120x3/6+15 header (bytes)
#define FRAME_SIZE_ENCODED 9600
#define HEADER_SIZE 15
//(FRAME_SIZE_ENCODED + HEADER_SIZE) / 512 is how many blocks and (FRAME_SIZE_ENCODED + HEADER_SIZE) % 512 is partial block
//so we write and read 113 blocks in our case with block 112 having partial valid data
#define NUM_BLOCKS 18
#define PARTIAL_BLOCK 399
unsigned char VIDEO_ENCODED[(FRAME_SIZE_ENCODED + HEADER_SIZE) * NUM_FRAMES];

#if LWIP_IPV6==1
void print_ip6(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf(" %x:%x:%x:%x:%x:%x:%x:%x\n\r",
			IP6_ADDR_BLOCK1(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK2(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK3(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK4(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK5(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK6(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK7(&ip->u_addr.ip6),
			IP6_ADDR_BLOCK8(&ip->u_addr.ip6));

}
#else
void
print_ip(char *msg, ip_addr_t *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void
print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}
#endif

#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
int ProgramSi5324(void);
int ProgramSfpPhy(void);
#endif
#endif

#ifdef XPS_BOARD_ZCU102
#ifdef XPAR_XIICPS_0_DEVICE_ID
int IicPhyReset(void);
#endif
#endif

//encoder/decoder fcns
#include "ccc_codec_ip.h"

#define ENC_START_ADDR	0x000
#define ENC_DONE_ADDR	0x004
#define ENC_RGB_ADDR	0x008
#define ENC_CCC_ADDR	0x048
#define DEC_START_ADDR	0x050
#define DEC_DONE_ADDR	0x054
#define DEC_CCC_ADDR	0x058
#define DEC_RGB_ADDR	0x060

#define NUM_ENCODES 1200
#define NUM_ENCODES_ROW 40
#define NUM_ENCODES_COL 30

static inline u32 read_reg(u32 offset) {
	u32 data = CCC_CODEC_IP_mReadReg (XPAR_CCC_CODEC_IP_0_S00_AXI_BASEADDR, offset);
	//xil_printf ("Reading register: address 0x%.3x, value 0x%.8x\n", offset, data);
	return data;
}

static inline void write_reg(u32 offset, u32 data) {
	//xil_printf ("Writing register: address 0x%.3x, value 0x%.8x\n", offset, data);
	CCC_CODEC_IP_mWriteReg (XPAR_CCC_CODEC_IP_0_S00_AXI_BASEADDR, offset, data);
//	xil_printf ("Done writing\n");
}


static inline void stop_enc() {
	write_reg(ENC_START_ADDR, 0);
}

static inline void start_enc() {
	write_reg(ENC_START_ADDR, 1);
}

static inline void wait_for_enc_done() {
	while (read_reg(ENC_DONE_ADDR) == 0) {
		continue;
	}
}

// y, x - 0 to 3
// r, g, b - 8-bit data
static inline void write_enc_rgb(int y, int x, u8 r, u8 g, u8 b) {
	int i_pixel = (y * 4) + x;
	int addr = ENC_RGB_ADDR + (i_pixel * 4);
	u32 data =
			(((u32) r) << 16) |
			(((u32) g) <<  8) |
			(((u32) b) <<  0);
	write_reg(addr, data);
}

// i - 0 or 1
static inline u32 read_enc_ccc(int i) {
	int addr = ENC_CCC_ADDR + (i * 4);
	u32 data = read_reg(addr);
	return data;
}


static inline void stop_dec() {
	write_reg(DEC_START_ADDR, 0);
}

static inline void start_dec() {
	write_reg(DEC_START_ADDR, 1);
}

static inline void wait_for_dec_done() {
	while (read_reg(DEC_DONE_ADDR) == 0) {
		continue;
	}
}

// i - 0 or 1
// data - 32-bit data
static inline void write_dec_ccc(int i, u32 data) {
	int addr = DEC_CCC_ADDR + (i * 4);
	write_reg(addr, data);
}

// y, x - 0 to 3
// r, g, b - 8-bit data, these are like 3 return values so need to pass pointers in
static inline void read_dec_rgb(int y, int x, u8* r, u8* g, u8* b) {
	int i_pixel = (y * 4) + x;
	int addr = DEC_RGB_ADDR + (i_pixel * 4);
	u32 data = read_reg(addr);
	*r = (data >> 16) & 0xFF;
	*g = (data >>  8) & 0xFF;
	*b = (data >>  0) & 0xFF;
}

void encode_video()
{
	//160x3
	int row_length = 480;
	unsigned int encoded;

	for (int frameIndex = 0; frameIndex < NUM_FRAMES; frameIndex++)
	{
		int index = 0;
		int indexEncoded = 0;

		//don't encode the header (15 bytes) and copy it to VIDEO_ENCODED
		for (; index < 15; index++, indexEncoded++)
		{
			VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)] = VIDEO[index+frameIndex*FRAME_SIZE_FULL];
		}

		//time to encode the rest of the frame (pixels now)
		for (int j = 0; j < NUM_ENCODES_COL; j++)
		{
			for (int i = 0; i < NUM_ENCODES_ROW; i++)
			{
				//we encode a 4x4 block so have to go to next rows in the image
				write_enc_rgb(0, 0, VIDEO[index+frameIndex*FRAME_SIZE_FULL], VIDEO[index+frameIndex*FRAME_SIZE_FULL+1], VIDEO[index+frameIndex*FRAME_SIZE_FULL+2]);
				write_enc_rgb(0, 1, VIDEO[index+frameIndex*FRAME_SIZE_FULL+3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+4], VIDEO[index+frameIndex*FRAME_SIZE_FULL+5]);
				write_enc_rgb(0, 2, VIDEO[index+frameIndex*FRAME_SIZE_FULL+6], VIDEO[index+frameIndex*FRAME_SIZE_FULL+7], VIDEO[index+frameIndex*FRAME_SIZE_FULL+8]);
				write_enc_rgb(0, 3, VIDEO[index+frameIndex*FRAME_SIZE_FULL+9], VIDEO[index+frameIndex*FRAME_SIZE_FULL+10], VIDEO[index+frameIndex*FRAME_SIZE_FULL+11]);

				write_enc_rgb(1, 0, VIDEO[index+frameIndex*FRAME_SIZE_FULL+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+1+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+2+row_length]);
				write_enc_rgb(1, 1, VIDEO[index+frameIndex*FRAME_SIZE_FULL+3+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+4+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+5+row_length]);
				write_enc_rgb(1, 2, VIDEO[index+frameIndex*FRAME_SIZE_FULL+6+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+7+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+8+row_length]);
				write_enc_rgb(1, 3, VIDEO[index+frameIndex*FRAME_SIZE_FULL+9+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+10+row_length], VIDEO[index+frameIndex*FRAME_SIZE_FULL+11+row_length]);

				write_enc_rgb(2, 0, VIDEO[index+frameIndex*FRAME_SIZE_FULL+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+1+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+2+row_length*2]);
				write_enc_rgb(2, 1, VIDEO[index+frameIndex*FRAME_SIZE_FULL+3+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+4+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+5+row_length*2]);
				write_enc_rgb(2, 2, VIDEO[index+frameIndex*FRAME_SIZE_FULL+6+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+7+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+8+row_length*2]);
				write_enc_rgb(2, 3, VIDEO[index+frameIndex*FRAME_SIZE_FULL+9+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+10+row_length*2], VIDEO[index+frameIndex*FRAME_SIZE_FULL+11+row_length*2]);

				write_enc_rgb(3, 0, VIDEO[index+frameIndex*FRAME_SIZE_FULL+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+1+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+2+row_length*3]);
				write_enc_rgb(3, 1, VIDEO[index+frameIndex*FRAME_SIZE_FULL+3+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+4+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+5+row_length*3]);
				write_enc_rgb(3, 2, VIDEO[index+frameIndex*FRAME_SIZE_FULL+6+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+7+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+8+row_length*3]);
				write_enc_rgb(3, 3, VIDEO[index+frameIndex*FRAME_SIZE_FULL+9+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+10+row_length*3], VIDEO[index+frameIndex*FRAME_SIZE_FULL+11+row_length*3]);

				start_enc();
				wait_for_enc_done();

				encoded = read_enc_ccc(0);

				//encoding gives us 8 bytes of data,  copy them to VIDEO_ENCODED
				for (int k = 0; k < 4; k++, indexEncoded++)
				{
					VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)] = (char)(encoded >> (k*8));
				}

				encoded = read_enc_ccc(1);

				for (int k = 0; k < 4; k++, indexEncoded++)
				{
					VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)] = (char)(encoded >> (k*8));
				}

				//move to next column in row
				index += 12;
			}

			//move 3 rows
			index += (row_length*3);
		}
	}
}

void decode_video()
{
	//160x3
	int row_length = 480;
	unsigned int decode_data;

	for (int frameIndex = 0; frameIndex < NUM_FRAMES; frameIndex++)
	{
		int index = 0;
		int indexEncoded = 0;

		//the header wasn't encoded, just copy it
		for (; index < 15; index++, indexEncoded++)
		{
			VIDEO[index+frameIndex*FRAME_SIZE_FULL] = VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)];
		}

		//time to decode the rest of the frame (pixels now)
		for (int j = 0; j < NUM_ENCODES_COL; j++)
		{
			for (int i = 0; i < NUM_ENCODES_ROW; i++)
			{
				decode_data = (VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)+3] << 24) |
							  (VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)+2] << 16) |
							  (VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)+1] << 8) |
							   VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)];

				write_dec_ccc(0, decode_data);

				decode_data = (VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)+7] << 24) |
						      (VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)+6] << 16) |
							  (VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)+5] << 8) |
							   VIDEO_ENCODED[indexEncoded+frameIndex*(FRAME_SIZE_ENCODED + HEADER_SIZE)+4];

				write_dec_ccc(1, decode_data);

				start_dec();
				wait_for_dec_done();

				for (int y = 0; y < 4; y++) {
					u8 r, g, b;
					read_dec_rgb(y, 0, &r, &g, &b);

					VIDEO[index+frameIndex*FRAME_SIZE_FULL+row_length*y] = r;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+1+row_length*y] = g;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+2+row_length*y] = b;

					read_dec_rgb(y, 1, &r, &g, &b);

					VIDEO[index+frameIndex*FRAME_SIZE_FULL+3+row_length*y] = r;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+4+row_length*y] = g;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+5+row_length*y] = b;

					read_dec_rgb(y, 2, &r, &g, &b);

					VIDEO[index+frameIndex*FRAME_SIZE_FULL+6+row_length*y] = r;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+7+row_length*y] = g;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+8+row_length*y] = b;

					read_dec_rgb(y, 3, &r, &g, &b);

					VIDEO[index+frameIndex*FRAME_SIZE_FULL+9+row_length*y] = r;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+10+row_length*y] = g;
					VIDEO[index+frameIndex*FRAME_SIZE_FULL+11+row_length*y] = b;
				}

				index += 12;
				indexEncoded += 8;
			}

			//move 3 rows
			index += (row_length*3);
		}
	}
}

int start_time()
{
	u32 Value1;
	u32 ControlStatus;

	/*
	 * Clear the Control Status Register
	 */
	XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0, 0x0);

	/*
	 * Set the value that is loaded into the timer counter and cause it to
	 * be loaded into the timer counter
	 */
	XTmrCtr_SetLoadReg(XPAR_TMRCTR_0_BASEADDR, 0, 0x0);
	XTmrCtr_LoadTimerCounterReg(XPAR_TMRCTR_0_BASEADDR, 0);

	/*
	 * Clear the Load Timer bit in the Control Status Register
	 */
	ControlStatus = XTmrCtr_GetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0);
	XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0, ControlStatus & (~XTC_CSR_LOAD_MASK));

	/*
	 * Get a snapshot of the timer counter value before it's started
	 * to compare against later
	 */
	Value1 = XTmrCtr_GetTimerCounterReg(XPAR_TMRCTR_0_BASEADDR, 0);

	/*
	 * Start the timer counter such that it's incrementing by default
	 */
	XTmrCtr_Enable(XPAR_TMRCTR_0_BASEADDR, 0);

	return Value1;
}

int stop_time()
{
	u32 Value2;

	Value2 = XTmrCtr_GetTimerCounterReg(XPAR_TMRCTR_0_BASEADDR, 0);

	XTmrCtr_Disable(XPAR_TMRCTR_0_BASEADDR, 0);

	return Value2;
}

int main()
{
	//to show we can read and write to sd card
	char block_data[512];

    for (int i = 0; i < 512; i = i + 4)
    {
    	*((unsigned int*)(block_data+i)) = 0x7F3E1D9C;
    }

    write_block(block_data,0);

    for (int i = 0; i < 512; i = i + 4)
    {
    	*((unsigned int*)(block_data+i)) = 0x6BE1F3A1;
    }

    write_block(block_data,1);

    read_block(block_data,0);

    for (int i = 0; i < 512; i = i + 4)
    {
    	xil_printf("Block data %x.\n\r",*((unsigned int*)(block_data+i)));
    }

    read_block(block_data,1);

    for (int i = 0; i < 512; i = i + 4)
    {
    	xil_printf("Block data %x.\n\r",*((unsigned int*)(block_data+i)));
    }

	//init uart first
	XUartLite *UartLiteInstPtr = &UartLiteInst;
	u16 UartLiteDeviceId = UARTLITE_DEVICE_ID;

	XUartLite_Initialize(UartLiteInstPtr, UartLiteDeviceId);

#if LWIP_IPV6==0
	ip_addr_t ipaddr, netmask, gw;

#endif
	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] =
	{ 0x00, 0x0a, 0x35, 0x00, 0x00, 0x22 };
	// Kanver - change above to match your board ^

	echo_netif = &server_netif;
#if defined (__arm__) && !defined (ARMR5)
#if XPAR_GIGE_PCS_PMA_SGMII_CORE_PRESENT == 1 || XPAR_GIGE_PCS_PMA_1000BASEX_CORE_PRESENT == 1
	ProgramSi5324();
	ProgramSfpPhy();
#endif
#endif

/* Define this board specific macro in order perform PHY reset on ZCU102 */
#ifdef XPS_BOARD_ZCU102
	if(IicPhyReset()) {
		xil_printf("Error performing PHY reset \n\r");
		return -1;
	}
#endif

	init_platform();

#if LWIP_IPV6==0
#if LWIP_DHCP==1
    ipaddr.addr = 0;
	gw.addr = 0;
	netmask.addr = 0;
#else
	/* initliaze IP addresses to be used */
	IP4_ADDR(&ipaddr,  192, 168,   1, 10);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   1,  1);
#endif
#endif
	print_app_header();

	lwip_init();

#if (LWIP_IPV6 == 0)
	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, &ipaddr, &netmask,
						&gw, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
#else
	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(echo_netif, NULL, NULL, NULL, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return -1;
	}
	echo_netif->ip6_autoconfig_enabled = 1;

	netif_create_ip6_linklocal_address(echo_netif, 1);
	netif_ip6_addr_set_state(echo_netif, 0, IP6_ADDR_VALID);

	print_ip6("\n\rBoard IPv6 address ", &echo_netif->ip6_addr[0].u_addr.ip6);

#endif
	netif_set_default(echo_netif);

	/* now enable interrupts */
	platform_enable_interrupts();

	//uart init
	XUartLite_SetSendHandler(UartLiteInstPtr, UartLiteSendHandler,
							 UartLiteInstPtr);

	XUartLite_SetRecvHandler(UartLiteInstPtr, UartLiteRecvHandler,
							 UartLiteInstPtr);

	XUartLite_EnableInterrupt(UartLiteInstPtr);

	/* specify that the network if is up */
	netif_set_up(echo_netif);

#if (LWIP_IPV6 == 0)
#if (LWIP_DHCP==1)
	/* Create a new DHCP client for this interface.
	 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	 * the predefined regular intervals after starting the client.
	 */
	dhcp_start(echo_netif);
	dhcp_timoutcntr = 24;

	while(((echo_netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0))
		xemacif_input(echo_netif);

	if (dhcp_timoutcntr <= 0) {
		if ((echo_netif->ip_addr.addr) == 0) {
			xil_printf("DHCP Timeout\r\n");
			xil_printf("Configuring default IP of 1.1.X.2\r\n");
			//Kanver - update ip_addr,netmask,gateway to match your board
			IP4_ADDR(&(echo_netif->ip_addr),  1, 1, 22, 2);
			IP4_ADDR(&(echo_netif->netmask), 255, 255, 0,  0);
			IP4_ADDR(&(echo_netif->gw),      1, 1,   0,  1);
		}
	}

	ipaddr.addr = echo_netif->ip_addr.addr;
	gw.addr = echo_netif->gw.addr;
	netmask.addr = echo_netif->netmask.addr;
#endif

	print_ip_settings(&ipaddr, &netmask, &gw);

#endif
	/* start the application (web server, rxtest, txtest, etc..) */
	//Kanver - added gracious ARP
	etharp_gratuitous(echo_netif);
	//start server
	start_application();
	//start client
	setup_client_conn();

	time1 = start_time();

	/* receive and process packets */
	while (1) {
		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}
		xemacif_input(echo_netif);
		transfer_data();

		//check if we've received the whole video
		//if so write to sd card
		if (frameNum == (FRAME_SIZE * NUM_FRAMES))
		{
			time2 = stop_time();
			//clock tick is 10ns b/c 1/100MHz
			//can divide by 100 to get units in us
			//can divide by 100 * 1000000 to get s
			//can divide by 100 * 1000 to get ms
			xil_printf("time video: %dms, bitrate is %dMbps\n", (time2-time1)/100000, FRAME_SIZE*NUM_FRAMES*8/((time2-time1)/100));
			#if CLIENT == 1
				time1 = start_time();
				decode_video();
				time2 = stop_time();
				xil_printf("time decode: %dms, bitrate is %dKbps\n", (time2-time1)/100000, FRAME_SIZE*NUM_FRAMES*8/((time2-time1)/100000));
			#else
				time1 = start_time();
				encode_video();
				time2 = stop_time();
				xil_printf("time encode: %dms, bitrate is %dMbps\n", (time2-time1)/100000, FRAME_SIZE*NUM_FRAMES*8/((time2-time1)/100));

				time1 = start_time();
				write_frames_to_sd();
				time2 = stop_time();
				//sd card writes encoded frame size
				xil_printf("time write to sd card: %dms, bitrate is %dMbps\n", (time2-time1)/100000, FRAME_SIZE_ENCODED*NUM_FRAMES*8/((time2-time1)/100));
			#endif
			//read_frames_from_sd();
			//xil_printf("Read frames from SD Card\n");
			//decode_video();
			//xil_printf("Decoded Video\n");
			frameNum = 0;
		}
	}

	/* never reached */
	cleanup_platform();

	return 0;
}

int setup_client_conn()
{
	struct tcp_pcb *pcb;
	err_t err;
	ip_addr_t remote_addr;

	xil_printf("Setting up client connection\n");

#if LWIP_IPV6==1
	remote_addr.type = IPADDR_TYPE_V6;
	err = inet6_aton(DEST_IP6_ADDR, &remote_addr);
#else
	err = inet_aton(DEST_IP4_ADDR, &remote_addr);
#endif

	if (!err) {
		xil_printf("Invalid Server IP address: %d\n", err);
		return -1;
	}

	//Create new TCP PCB structure
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n");
		return -1;
	}

	//Bind to specified @port
	err = tcp_bind(pcb, IP_ANY_TYPE, SRC_PORT);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n", SRC_PORT, err);
		return -2;
	}

	//Connect to remote server (with callback on connection established)
	err = tcp_connect(pcb, &remote_addr, DEST_PORT, tcp_client_connected);
	if (err) {
		xil_printf("Error on tcp_connect: %d\n", err);
		tcp_client_close(pcb);
		return -1;
	}

	is_connected = 0;

	xil_printf("Waiting for server to accept connection\n");

	return 0;
}

static void tcp_client_close(struct tcp_pcb *pcb)
{
	err_t err;

	xil_printf("Closing Client Connection\n");

	if (pcb != NULL) {
		tcp_sent(pcb, NULL);
		tcp_recv(pcb,NULL);
		tcp_err(pcb, NULL);
		err = tcp_close(pcb);
		if (err != ERR_OK) {
			/* Free memory with abort */
			tcp_abort(pcb);
		}
	}
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	if (err != ERR_OK) {
		tcp_client_close(tpcb);
		xil_printf("Connection error\n");
		return err;
	}

	xil_printf("Connection to server established\n");

	//Store state (for callbacks)
	c_pcb = tpcb;
	is_connected = 1;

	//Set callback values & functions
	tcp_arg(c_pcb, NULL);
	tcp_recv(c_pcb, tcp_client_recv);
	tcp_sent(c_pcb, tcp_client_sent);
	tcp_err(c_pcb, tcp_client_err);

	return ERR_OK;
}

static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	//If no data, connection closed
	if (!p) {
		xil_printf("No data received\n");
		tcp_client_close(tpcb);
		return ERR_OK;
	}

	//ADD CODE HERE to do on packet reception
	u32_t i;

	for (struct pbuf * ptr = p; ptr != NULL; ptr = ptr->next)
	{
		for(i = 0; i < ptr->len; i = i + 1)
		{
			#if CLIENT == 1
				VIDEO_ENCODED[i + frameNum] = *(((char*)ptr->payload)+i);
			#else
				VIDEO[i + frameNum] = *(((char*)ptr->payload)+i);
			#endif
		}
		frameNum += ptr->len;
	}

	//check if we've received one whole frame yet
	if (frameNum % FRAME_SIZE == 0)
	{
		//if we've received a whole video end stream
		if (frameNum == (FRAME_SIZE * NUM_FRAMES))
		{
			//Indicate done processing
			tcp_recved(tpcb, p->tot_len);
			//Free the received pbuf
			pbuf_free(p);
			xil_printf("Done Video Transfer\n");
			tcp_client_close(tpcb);
			return ERR_OK;
		}

		//Just send a single packet
		u8_t apiflags = TCP_WRITE_FLAG_COPY;

	    char send_buf[16];
		u32_t i;

		for(i = 0; i < 15; i = i + 1)
		{
			send_buf[i] = i+'a';
		}
		send_buf[15] = '\n';

		//Loop until enough room in buffer (should be right away)
		while (tcp_sndbuf(c_pcb) < 16);

		//Enqueue some data to send
		err = tcp_write(c_pcb, send_buf, 16, apiflags);
		if (err != ERR_OK) {
			xil_printf("TCP client: Error on tcp_write: %d\n", err);
			return err;
		}

		//send the data packet
		err = tcp_output(c_pcb);
		if (err != ERR_OK) {
			xil_printf("TCP client: Error on tcp_output: %d\n",err);
			return err;
		}

		xil_printf("Frame sent\n");
	}

	//END OF ADDED CODE

	//Indicate done processing
	tcp_recved(tpcb, p->tot_len);

	//Free the received pbuf
	pbuf_free(p);

	return 0;
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	//ADD CODE HERE to do on packet acknowledged

	//Print message
	//xil_printf("Packet sent successfully, %d bytes\n", len);

	//END OF ADDED CODE

	return 0;
}

static void tcp_client_err(void *arg, err_t err)
{
	LWIP_UNUSED_ARG(err);
	tcp_client_close(c_pcb);
	c_pcb = NULL;
	xil_printf("TCP connection aborted\n");
}

static void UartLiteSendHandler(void *CallBackRef, unsigned int EventData)
{

}

//https://pythonhosted.org/pyserial/pyserial_api.html
//https://github.com/Xilinx/embeddedsw/blob/master/XilinxProcessorIPLib/drivers/uartlite/examples/xuartlite_intr_tapp_example.c
static void UartLiteRecvHandler(void *CallBackRef, unsigned int EventData)
{
	char data[2];
	XUartLite_Recv((XUartLite *)CallBackRef,(u8*)data,1);
	data[1] = 0;
	xil_printf("Received data from terminal %c\n",data[0]);
}

void write_frames_to_sd()
{
	char block_data[512];
	unsigned int totalFrameSize = FRAME_SIZE_ENCODED + HEADER_SIZE;
	unsigned int totalNumBlocks = NUM_BLOCKS+1;

	for (int nFrame = 0; nFrame < NUM_FRAMES; nFrame++)
	{
		int nFrameIndex = 0;

		for (; nFrameIndex < NUM_BLOCKS; nFrameIndex++)
		{
			for (int i = 0; i < 512; i++)
			{
				block_data[i] = VIDEO_ENCODED[i+(nFrameIndex*512)+(nFrame*totalFrameSize)];
			}

			write_block(block_data,nFrameIndex+nFrame*totalNumBlocks);
		}

		//fill up the remaining bytes in last block
		for (int i = 0; i < PARTIAL_BLOCK; i++)
		{
			block_data[i] = VIDEO_ENCODED[i+(nFrameIndex*512)+(nFrame*totalFrameSize)];
		}

		write_block(block_data,nFrameIndex+nFrame*totalNumBlocks);
	}
}

void read_frames_from_sd()
{
	char block_data[512];
	unsigned int totalFrameSize = FRAME_SIZE_ENCODED + HEADER_SIZE;
	unsigned int totalNumBlocks = NUM_BLOCKS+1;

	for (int nFrame = 0; nFrame < NUM_FRAMES; nFrame++)
	{
		int nFrameIndex = 0;

		for (; nFrameIndex < NUM_BLOCKS; nFrameIndex++)
		{
			read_block(block_data,nFrameIndex+nFrame*totalNumBlocks);

			for (int i = 0; i < 512; i++)
			{
				VIDEO_ENCODED[i+(nFrameIndex*512)+(nFrame*totalFrameSize)] = block_data[i];
			}
		}

		//read the last partial block into a temp array and only copy over 271 bytes
		read_block(block_data,nFrameIndex+nFrame*totalNumBlocks);

		for (int i = 0; i < PARTIAL_BLOCK; i++)
		{
			VIDEO_ENCODED[i+(nFrameIndex*512)+(nFrame*totalFrameSize)] = block_data[i];
		}
	}
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
    	continue;
    }

    SD_MYCODE_mWriteReg(sdcard,START_WRITE,1);

    while ( SD_MYCODE_mReadReg(sdcard,DONE_WRITE) != 0x1)
    {
    	continue;
    }

    SD_MYCODE_mWriteReg(sdcard,DONE_WRITE,0);
}

static inline void read_block(char* block, unsigned int addr)
{
    //do a read of block addr
    SD_MYCODE_mWriteReg(sdcard,RW_ADDR,addr);

    while ( SD_MYCODE_mReadReg(sdcard,SD_READY) != 0x1)
    {
    	continue;
    }

    SD_MYCODE_mWriteReg(sdcard,START_READ,1);

    while ( SD_MYCODE_mReadReg(sdcard,DONE_READ) != 0x1)
    {
    	continue;
    }

    SD_MYCODE_mWriteReg(sdcard,DONE_READ,0);

    //store the read out data in block
    for (int i = 0; i < 512; i = i + 4)
    {
    	*((unsigned int*)(block+i)) = SD_MYCODE_mReadReg(sdcard,i);
    }
}
