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
#include <string.h>

#include "lwip/err.h"
#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

//set to 0 for server and 1 for client
#define CLIENT 0

//160x120x3+15 header (bytes)
#if CLIENT == 1
	#define FRAME_SIZE 57615
#else
//160x120x3/6+15 header (bytes)
	#define FRAME_SIZE 9615
#endif

//NUM_FRAMES should be 137
#define NUM_FRAMES 137
//how much we send in a tcp packet
#define TCP_PACKET_SIZE 8192
//number of packets we have to send to send a whole frame (not including header)
//FRAME_SIZE/TCP_PACKET_SIZE = NUM_PACKETS_IN_FRAME (rounded down to int)
#if CLIENT == 1
	#define NUM_PACKETS_IN_FRAME 7
#else
	#define NUM_PACKETS_IN_FRAME 1
#endif
//as /FRAME_SIZE/TCP_PACKET_SIZE can be a decimal have to send left over frames
#if CLIENT == 1
	#define LEFT_OVER_BYTES 271
#else
	#define LEFT_OVER_BYTES 1423
#endif
#define FRAME_SIZE_FULL 56715
extern unsigned char VIDEO[FRAME_SIZE_FULL * NUM_FRAMES];
int framePacketsSent;
int frameBytesSent;
//large static global array to hold encoded frame data
//160x120x3/6+15 header (bytes)
#define FRAME_SIZE_ENCODED 9600
#define HEADER_SIZE 15
extern unsigned char VIDEO_ENCODED[(FRAME_SIZE_ENCODED + HEADER_SIZE) * NUM_FRAMES];

int transfer_data() {
	return 0;
}

void print_app_header()
{
#if (LWIP_IPV6==0)
	xil_printf("\n\r\n\r-----lwIP TCP echo server ------\n\r");
#else
	xil_printf("\n\r\n\r-----lwIPv6 TCP echo server ------\n\r");
#endif
	xil_printf("TCP packets sent to port 50000 will be echoed back\n\r");
}

err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{
	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);
	xil_printf("Frame bytes successfully, %d\n", frameBytesSent);

	if (frameBytesSent != FRAME_SIZE * NUM_FRAMES)
	{
		u8_t apiflags = 0x0;

		//Loop until enough room in buffer (should be right away)
		//TCP_SND_BUF is too large it causes issues, for now 2048 works but try other values
		while (tcp_sndbuf(tpcb) < TCP_PACKET_SIZE)
		{
			continue;
		}

		//Enqueue some data to send
		#if CLIENT == 1
			err = tcp_write(tpcb, VIDEO+frameBytesSent, TCP_PACKET_SIZE, apiflags);
		#else
			err = tcp_write(tpcb, VIDEO_ENCODED+frameBytesSent, TCP_PACKET_SIZE, apiflags);
		#endif
		if (err != ERR_OK) {
			xil_printf("TCP client: Error on tcp_write: %d\n", err);
			return err;
		}

		//send the data packet
		err = tcp_output(tpcb);
		if (err != ERR_OK) {
			xil_printf("TCP client: Error on tcp_output: %d\n",err);
			return err;
		}

		//xil_printf("Packet data sent\n");

		frameBytesSent += TCP_PACKET_SIZE;
		framePacketsSent++;
	}

	/* free the received pbuf */
	pbuf_free(p);

	return ERR_OK;
}

static err_t sent_callback(void *arg, struct tcp_pcb *newpcb, u16_t len)
{
	//ADD CODE HERE to do on packet acknowledged
	u8_t apiflags = 0x0;
	err_t err;

	//xil_printf("sent callback triggered\n");

	if (framePacketsSent != 0)
	{
		//change this to match your tcp packet size (450 for 2048 byte sized packets, and 1800 for 512 byte sized packets with frame size 640x480)
		if (framePacketsSent == NUM_PACKETS_IN_FRAME)
		{
			//Loop until enough room in buffer (should be right away)
			//send remaining data (as we don't send a full TCP_PACKET_SIZE)
			//NUM_PACKETS_IN_FRAME = 112 but we have 0.5 NUM_PACKETS_IN_FRAME plus 15 byte header left
			//so that is 271 or LEFT_OVER_BYTES
			while (tcp_sndbuf(newpcb) < LEFT_OVER_BYTES)
			{
				continue;
			}

			//Enqueue some data to send
			#if CLIENT == 1
				err = tcp_write(newpcb, VIDEO+frameBytesSent, LEFT_OVER_BYTES, apiflags);
			#else
				err = tcp_write(newpcb, VIDEO_ENCODED+frameBytesSent, LEFT_OVER_BYTES, apiflags);
			#endif
			if (err != ERR_OK) {
				xil_printf("TCP client: Error on tcp_write: %d\n", err);
				return err;
			}

			//send the data packet
			err = tcp_output(newpcb);
			if (err != ERR_OK) {
				xil_printf("TCP client: Error on tcp_output: %d\n",err);
				return err;
			}

			frameBytesSent += LEFT_OVER_BYTES;
			framePacketsSent = 0;
		}
		else
		{
			//Loop until enough room in buffer (should be right away)
			while (tcp_sndbuf(newpcb) < TCP_PACKET_SIZE)
			{
				continue;
			}

			//Enqueue some data to send
			#if CLIENT == 1
				err = tcp_write(newpcb, VIDEO+frameBytesSent, TCP_PACKET_SIZE, apiflags);
			#else
				err = tcp_write(newpcb, VIDEO_ENCODED+frameBytesSent, TCP_PACKET_SIZE, apiflags);
			#endif
			if (err != ERR_OK) {
				xil_printf("TCP client: Error on tcp_write: %d\n", err);
				return err;
			}

			//send the data packet
			err = tcp_output(newpcb);
			if (err != ERR_OK) {
				xil_printf("TCP client: Error on tcp_output: %d\n",err);
				return err;
			}

			//xil_printf("Packet data sent\n");

			frameBytesSent += TCP_PACKET_SIZE;
			framePacketsSent++;
		}

		//Print message
		//xil_printf("Packet sent successfully, %d bytes\n", len);
	}

	//END OF ADDED CODE

	return 0;
}

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	static int connection = 1;

	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);
	tcp_sent(newpcb, sent_callback);

	/* just use an integer number indicating the connection id as the
	   callback argument */
	tcp_arg(newpcb, (void*)(UINTPTR)connection);

	/* increment for subsequent accepted connections */
	connection++;

	//send a single packet
	u8_t apiflags = 0x0;

	//Loop until enough room in buffer (should be right away)
	//TCP_SND_BUF is too large it causes issues, for now 2048 works but try other values
	while (tcp_sndbuf(newpcb) < TCP_PACKET_SIZE)
	{
		continue;
	}

	//Enqueue some data to send
	#if CLIENT == 1
		err = tcp_write(newpcb, VIDEO+frameBytesSent, TCP_PACKET_SIZE, apiflags);
	#else
		err = tcp_write(newpcb, VIDEO_ENCODED+frameBytesSent, TCP_PACKET_SIZE, apiflags);
	#endif
	if (err != ERR_OK) {
		xil_printf("TCP client: Error on tcp_write: %d\n", err);
		return err;
	}

	//send the data packet
	err = tcp_output(newpcb);
	if (err != ERR_OK) {
		xil_printf("TCP client: Error on tcp_output: %d\n",err);
		return err;
	}

	frameBytesSent += TCP_PACKET_SIZE;
	framePacketsSent++;

	return ERR_OK;
}


int start_application()
{
	struct tcp_pcb *pcb;
	err_t err;
	//Kanver - change port to match
	unsigned port = 50000;

	/* create new TCP PCB structure */
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ANY_TYPE, port);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callback to use for incoming connections */
	tcp_accept(pcb, accept_callback);

	xil_printf("TCP echo server started @ port %d\n\r", port);

	return 0;
}
