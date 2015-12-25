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
 *  $Id: sv_io_select.c,v 1.6 2004/02/18 05:31:50 rzhe Exp $
 */

#ifndef _WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#else
#include <winsock.h>
#define STDIN_FILENO 0
#endif
#include <string.h>

#include "qwsvdef.h"
#include "sv_io.h"


static fd_set readfds, writefds, tmp_readfds, tmp_writefds;
static int sockfd;
static struct timeval tout, tmp_tout;


void IO_Engine_Init(int socket, int do_stdin, int timeout)
{
	sockfd = socket;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	FD_SET(sockfd, &readfds);
	if (do_stdin)
		FD_SET(STDIN_FILENO, &readfds);

	tout.tv_sec = timeout / 1000;
	tout.tv_usec = timeout % 1000 * 1000;
}

void IO_Engine_Shutdown()
{
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
}

void IO_Engine_Query()
{
	int num;
#ifdef _WIN32
	int err;
#endif
	memcpy(&tmp_readfds, &readfds, sizeof(fd_set));
	memcpy(&tmp_writefds, &writefds, sizeof(fd_set));
	memcpy(&tmp_tout, &tout, sizeof(tout));

	do
	{
		num = select(sockfd + 1, &tmp_readfds, &tmp_writefds,
			NULL, &tmp_tout);

		switch (num)
		{
		case -1:
#ifndef _WIN32
			if (errno != EINTR)
#else
			err = WSAGetLastError();
			if (err == WSAEINTR || err == WSAEINPROGRESS)
#endif
			{
#ifndef _WIN32
				Log_Printf(SV_ERRORLOG, "IO_Engine_Query: "
					"select() failed -- %d, %s\n", errno,
					strerror(errno));
#else
				Log_Printf(SV_ERRORLOG, "IO_Engine_Query: "
					"select() failed -- %d\n", err);
#endif
				IO_status |= IO_QUERY_ERROR;
				return;
			}
			break;
		case 0:
			return;
		default:
			break;
		}
	}
	while (num == -1);

	if (FD_ISSET(STDIN_FILENO, &tmp_readfds))
		IO_status |= IO_STDIN_AVAIL;

	if (FD_ISSET(sockfd, &tmp_readfds))
		IO_status |= IO_SOCKREAD_AVAIL;

	if (FD_ISSET(sockfd, &tmp_writefds))
		IO_status |= IO_SOCKWRITE_AVAIL;
}

void IO_Engine_MarkWrite(int mark)
{
	if (mark)
		FD_SET(sockfd, &writefds);
	else
#ifdef _WIN32
		FD_CLR((SOCKET) sockfd, &writefds);
#else
		FD_CLR(sockfd, &writefds);
#endif
}
