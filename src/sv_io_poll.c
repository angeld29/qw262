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
 *  $Id: sv_io_poll.c,v 1.3 2004/02/18 05:31:50 rzhe Exp $
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

#include "qwsvdef.h"
#include "sv_io.h"


static struct pollfd pfds[2];
static int nfds = 0;
static int sockfd;
static int tout;


void IO_Engine_Init(int socket, int do_stdin, int timeout)
{
	sockfd = socket;

	pfds[0].fd = sockfd;
	pfds[0].events = POLLIN;
	nfds = 1;

	if (do_stdin)
	{
		pfds[1].fd = STDIN_FILENO;
		pfds[1].events = POLLIN;
		nfds = 2;
	}

	tout = timeout;
}

void IO_Engine_Shutdown()
{
	memset(pfds, 0, sizeof(pfds));
	nfds = 0;
}

void IO_Engine_Query()
{
	int num;

	do
	{
		num = poll(pfds, nfds, tout);

		switch (num)
		{
		case -1:
			if (errno != EINTR)
			{
				Log_Printf(SV_ERRORLOG, "IO_Engine_Query: "
					"poll() failed -- %d, %s\n", errno,
					strerror(errno));
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

	if (pfds[0].revents & POLLIN)
		IO_status |= IO_SOCKREAD_AVAIL;

	if (pfds[0].revents & POLLOUT)
		IO_status |= IO_SOCKWRITE_AVAIL;

	if (pfds[1].revents & POLLIN)
		IO_status |= IO_STDIN_AVAIL;
}

void IO_Engine_MarkWrite(int mark)
{
	if (mark)
		pfds[0].events |= POLLOUT;
	else
		pfds[0].events &= ~POLLOUT;
}
