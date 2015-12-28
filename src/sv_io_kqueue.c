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
 *  $Id: sv_io_kqueue.c,v 1.4 2004/02/18 05:31:50 rzhe Exp $
 */

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "qwsvdef.h"
#include "sv_io.h"


static const struct timespec zerotime = { 0, 0 };

static int kq;
static struct kevent kevs[3];
static int sockfd;
static struct timespec tout;


void IO_Engine_Init(int socket, int do_stdin, int timeout)
{
	int i = 0;

	sockfd = socket;

	if ((kq = kqueue()) == -1)
		SV_Error("IO_Engine_Init: kqueue() failed");

	EV_SET(&kevs[i++], (intptr_t) sockfd, EVFILT_READ, EV_ADD, 0, 0, 0);

	if (do_stdin)
		EV_SET(&kevs[i++], (intptr_t) STDIN_FILENO, EVFILT_READ,
			EV_ADD, 0, 0, 0);

	if (kevent(kq, kevs, i, NULL, 0, &zerotime) == -1)
		SV_Error("IO_Engine_Init: kevent() failed");

	tout.tv_sec = timeout / 1000 ;
	tout.tv_nsec = timeout % 1000 * 1000000;
}

void IO_Engine_Shutdown()
{
	EV_SET(&kevs[0], (intptr_t) STDIN_FILENO, EVFILT_READ, EV_DELETE, 0,
		0, 0);
	EV_SET(&kevs[1], (intptr_t) sockfd, EVFILT_READ, EV_DELETE, 0, 0, 0);
	EV_SET(&kevs[2], (intptr_t) sockfd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);

	kevent(kq, kevs, 3, NULL, 0, &zerotime);
}

void IO_Engine_Query()
{
	int num, i, fd;

	do
	{
		num = kevent(kq, NULL, 0, kevs, 3, &tout);

		switch (num)
		{
		case -1:
			if (errno != EINTR)
			{
				Log_Printf(SV_ERRORLOG, "IO_Engine_Query: "
					"kevent() failed -- %d, %s\n", errno,
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

	for (i = 0; i < num; i++)
	{
		fd = (int) kevs[i].ident;

		if (fd == STDIN_FILENO)
			IO_status |= IO_STDIN_AVAIL;
		else if (kevs[i].filter == EVFILT_READ)
			IO_status |= IO_SOCKREAD_AVAIL;
		else
			IO_status |= IO_SOCKWRITE_AVAIL;
	}
}

void IO_Engine_MarkWrite(int mark)
{
	if (mark)
	{
		EV_SET(&kevs[0], (intptr_t) sockfd, EVFILT_WRITE,
			EV_ADD | EV_ONESHOT, 0, 0, 0);

		if (kevent(kq, kevs, 1, NULL, 0, &zerotime) == -1)
			Log_Printf(SV_ERRORLOG, "IO_Engine_MarkWrite: "
				"kevent() failed\n");
	}
}
