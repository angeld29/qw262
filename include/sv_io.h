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
 *  $Id: sv_io.h,v 1.4 2004/02/18 05:31:50 rzhe Exp $
 */

#ifndef __SV_IO_H__
#define __SV_IO_H__


#define	IO_STDIN_AVAIL		0x0001
#define	IO_SOCKREAD_AVAIL	0x0002
#define IO_SOCKWRITE_AVAIL	0x0004
#define IO_QUERY_ERROR		0x0008
#define IO_SENDQ_EXCEEDED	0x0010

#define IO_QUERY_STATUS		(IO_STDIN_AVAIL | IO_SOCKREAD_AVAIL | \
				IO_SOCKWRITE_AVAIL | IO_QUERY_ERROR)
#define IO_QUEUE_STATUS		IO_SENDQ_EXCEEDED


typedef int (*sendcallback_t)(const unsigned char *data, unsigned size,
		const struct sockaddr_in *addr);


extern int IO_status;


extern void IO_Init(int socket, int do_stdin, sendcallback_t scb,
		int timeout); /* timeout in milliseconds */
extern void IO_Shutdown();
extern void IO_Query();
extern void IO_Queue(const unsigned char *data, unsigned size,
		const struct sockaddr_in *addr);

extern void IO_Engine_Init(int socket, int do_stdin, int timeout);
extern void IO_Engine_Shutdown();
extern void IO_Engine_Query();
extern void IO_Engine_MarkWrite(int mark);


#endif /* !__SV_IO_H__ */
