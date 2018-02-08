/*
 *  QW262
 *  Copyright (C) 2003  [MAD]ApxuTekTop
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: sv_io.c,v 1.5 2004/02/18 05:31:50 rzhe Exp $
 */

#ifndef _WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#else
#include <winsock.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "qwsvdef.h"
#include "sv_io.h"


#define SENDQ_MAXSIZE		0x00080000	/* 512K */

#define PACKET_FLAGS_SPACE	0x0001

#define PACKET_OFFSET_FLAGS	(0)
#define PACKET_OFFSET_ADDR	(PACKET_OFFSET_FLAGS + sizeof(int))
#define PACKET_OFFSET_SIZE	(PACKET_OFFSET_ADDR + sizeof(struct sockaddr_in))
#define PACKET_OFFSET_DATA	(PACKET_OFFSET_SIZE + sizeof(unsigned))

#define PACKET_HEADER_SIZE	PACKET_OFFSET_DATA


int IO_status = 0;


static unsigned char *SendQ = NULL;
static unsigned sendq_head, sendq_tail, sendq_size;

static sendcallback_t sendcallback;


static void SendQueue(int flush);


void IO_Init(int socket, int do_stdin, sendcallback_t scb, int timeout)
{
	if (!(SendQ = (unsigned char *) malloc(SENDQ_MAXSIZE)))
		SV_Error("IO_Init: out of memory");

	sendq_head = sendq_tail = sendq_size = 0;

	sendcallback = scb;

	IO_Engine_Init(socket, do_stdin, timeout);
}

void IO_Shutdown()
{
	SendQueue(1);

	IO_Engine_Shutdown();

	if (SendQ)
		free(SendQ);
}

void IO_Query()
{
	IO_status &= ~IO_QUERY_STATUS;

	IO_Engine_Query();

	if (IO_status & IO_SOCKWRITE_AVAIL)
	{
		SendQueue(0);

		IO_Engine_MarkWrite(sendq_size);
	}
}

void IO_Queue(const unsigned char *data, unsigned size,
		const struct sockaddr_in *addr)
{
	unsigned packetsize, rightspace, maxbusyspace, psize;
	unsigned char *packet;

	IO_status &= ~IO_QUEUE_STATUS;
	
	packetsize = size + PACKET_HEADER_SIZE;
	rightspace = SENDQ_MAXSIZE - sendq_tail;
	maxbusyspace = SENDQ_MAXSIZE - packetsize;

	if (packetsize > rightspace)
	{
		if (sendq_head > sendq_tail)
		{
			do
			{
				psize = *((unsigned *) (SendQ + sendq_head +
					PACKET_OFFSET_SIZE));
				psize += PACKET_HEADER_SIZE;

				sendq_head += psize;

				sendq_size -= psize;
			}
			while (sendq_head != SENDQ_MAXSIZE);

			sendq_head = 0;

			IO_status |= IO_SENDQ_EXCEEDED;
		}

		packet = SendQ + sendq_tail;
		*((int *) (packet + PACKET_OFFSET_FLAGS)) = PACKET_FLAGS_SPACE;
		*((unsigned *) (packet + PACKET_OFFSET_FLAGS)) =
			rightspace - PACKET_HEADER_SIZE;

		sendq_tail = 0;

		sendq_size += rightspace;
	}

	if (sendq_size > maxbusyspace)
	{
		do
		{
			psize = *((unsigned *) (SendQ +	sendq_head +
				PACKET_OFFSET_SIZE));
			psize += PACKET_HEADER_SIZE;

			sendq_head += psize;

			sendq_size -= psize;
		}
		while (sendq_size > maxbusyspace);

		if (sendq_head == SENDQ_MAXSIZE)
			sendq_head = 0;

		IO_status |= IO_SENDQ_EXCEEDED;
	}

	packet = SendQ + sendq_tail;
	*((int *) (packet + PACKET_OFFSET_FLAGS)) = 0;
	memcpy(packet + PACKET_OFFSET_ADDR, addr, sizeof(struct sockaddr_in));
	*((unsigned *) (packet + PACKET_OFFSET_SIZE)) = size;
	memcpy(packet + PACKET_OFFSET_DATA, data, size);

	if ((sendq_tail += packetsize) == SENDQ_MAXSIZE)
		sendq_tail = 0;

	sendq_size += packetsize;

	IO_Engine_MarkWrite(1);
}

static void SendQueue(int flush)
{
	unsigned char *packet, *data;
	unsigned size, packetsize;
	int flags, sent = 1;

	while (sendq_size && (sent || flush))
	{
		packet = SendQ + sendq_head;
		flags = *((int *) (packet + PACKET_OFFSET_FLAGS));
		size = *((unsigned *) (packet + PACKET_OFFSET_SIZE));
		packetsize = size + PACKET_HEADER_SIZE;
		data = packet + PACKET_OFFSET_DATA;

		if (!(flags & PACKET_FLAGS_SPACE))
			sent = (*sendcallback)(data, size,
				(struct sockaddr_in *) (packet +
					PACKET_OFFSET_ADDR));

		if (sent)
		{
			if ((sendq_head += packetsize) == SENDQ_MAXSIZE)
				sendq_head = 0;

			sendq_size -= packetsize;
		}
	}

	if (!sendq_size)
		sendq_head = sendq_tail = 0;
}
