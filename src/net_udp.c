/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_main.c

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>

#ifdef OS_SOLARIS
#include <unistd.h>
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#ifdef SERVERONLY
#include "sv_io.h"
#endif

netadr_t	net_local_adr;

netadr_t	net_from;
sizebuf_t	net_message;
int		net_socket;			// non blocking, for receives

byte		net_message_buffer[MSG_BUF_SIZE];

//int gethostname (char *, int);
//int close (int);

//=============================================================================

void NetadrToSockadr (netadr_t *a, struct sockaddr_in *s)
{
	memset (s, 0, sizeof(*s));
	s->sin_family = AF_INET;

	*(int *)&s->sin_addr = *(int *)&a->ip;
	s->sin_port = a->port;
}

void SockadrToNetadr (struct sockaddr_in *s, netadr_t *a)
{
	*(int *)&a->ip = *(int *)&s->sin_addr;
	a->port = s->sin_port;
}

qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
		return true;
	return false;
}


qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
	
	sprintf (s, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}

char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];
	
	sprintf (s, "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
NET_StringToAdr

idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	struct hostent	*h;
	struct sockaddr_in sadr;
	char	*colon;
	char	copy[128];
	
	
	memset (&sadr, 0, sizeof(sadr));
	sadr.sin_family = AF_INET;
	
	sadr.sin_port = 0;

	strcpy (copy, s);
	// strip off a trailing :port if present
	for (colon = copy ; *colon ; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			sadr.sin_port = htons(atoi(colon+1));	
		}
	
	if (copy[0] >= '0' && copy[0] <= '9')
	{
		*(int *)&sadr.sin_addr = inet_addr(copy);
	}
	else
	{
		if (! (h = gethostbyname(copy)) )
			return 0;
		*(int *)&sadr.sin_addr = *(int *)h->h_addr_list[0];
	}
	
	SockadrToNetadr (&sadr, a);

	return true;
}

// Returns true if we can't bind the address locally--in other words, 
// the IP is NOT one of our interfaces.
qboolean NET_IsClientLegal(netadr_t *adr)
{
#if 0
	struct sockaddr_in sadr;
	int newsocket;

	if (adr->ip[0] == 127)
		return false; // no local connections period

	NetadrToSockadr (adr, &sadr);

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		Sys_Error ("NET_IsClientLegal: socket:", strerror(errno));

	sadr.sin_port = 0;

	if( bind (newsocket, (void *)&sadr, sizeof(sadr)) == -1) 
	{
		// It is not a local address
		close(newsocket);
		return true;
	}
	close(newsocket);
	return false;
#else
	return true;
#endif
}


//=============================================================================

#ifdef QW262
#include "net262.inc"
#endif

qboolean NET_GetPacket (void)
{
	int 	ret;
	struct	sockaddr_in	from;
	int		fromlen;

	fromlen = sizeof(from);
	ret = recvfrom (net_socket, net_message_buffer, sizeof(net_message_buffer), 0, (struct sockaddr *)&from, &fromlen);
	if (ret == -1) {
		if (errno == EWOULDBLOCK)
			return false;
		if (errno == ECONNREFUSED)
			return false;
		Sys_Printf ("NET_GetPacket: %s\n", strerror(errno));
		return false;
	}
#ifndef SERVERONLY
	else
		net_traffic_in += (ret + 28);
#endif

	if(ret<MSG_BUF_SIZE) net_message_buffer[ret]='\0';


#ifdef QW262
	{
		int i;
		if(IsMDserver)
			GetPacket262();
	}
#endif

	net_message.cursize = ret;
	SockadrToNetadr (&from, &net_from);

	return ret;
}

//=============================================================================
#ifdef SERVERONLY
extern qboolean qpluginCommand;
#endif

void NET_SendPacket (int length, void *data, netadr_t to)
{
#ifndef SERVERONLY
	int			ret;
#endif
	struct	sockaddr_in	addr;

#ifdef QW262
#ifdef SERVERONLY
	if(!qpluginCommand)
#endif
	{
		int i;
		if(IsMDserver)
			SendPacket262();
	}
#endif	

	NetadrToSockadr (&to, &addr);

#ifdef SERVERONLY
	IO_Queue(data, length, &addr);
	if (IO_status & IO_SENDQ_EXCEEDED)
		Sys_Printf ("NET_SendPacket: SendQ exceeded\n");
#else
	ret = sendto (net_socket, data, length, 0, (struct sockaddr *)&addr, sizeof(addr) );

	if (ret == -1) {
		if (errno == EWOULDBLOCK)
			return;
		if (errno == ECONNREFUSED)
			return;
		Sys_Printf ("NET_SendPacket: %s\n", strerror(errno));
	} else
		net_traffic_out += (ret + 28);
#endif /* SERVERONLY */
}

#ifdef SERVERONLY
int NET_Sendto(const unsigned char *data, unsigned size,
		const struct sockaddr_in *addr)
{
	int ret = sendto(net_socket, data, size, 0, (struct sockaddr *) addr,
		sizeof (struct sockaddr_in));

	if (ret == -1)
	{
		if (errno == EWOULDBLOCK)
			return 0;
		if (errno == ECONNREFUSED)
			return 1;
		Sys_Printf ("NET_Sendto: %s\n", strerror(errno));
	}

	return 1;
}
#endif /* SERVERONLY */


//=============================================================================

int UDP_OpenSocket (int port)
{
	int newsocket;
	struct sockaddr_in address;
	qboolean _true = true;
	int i;

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		Sys_Error ("UDP_OpenSocket: socket:", strerror(errno));
	if (ioctl (newsocket, FIONBIO, (char *)&_true) == -1)
		Sys_Error ("UDP_OpenSocket: ioctl FIONBIO:", strerror(errno));
	address.sin_family = AF_INET;
//ZOID -- check for interface binding option
	if ((i = COM_CheckParm("-ip")) != 0 && i < com_argc) {
		address.sin_addr.s_addr = inet_addr(com_argv[i+1]);
		Con_Printf("Binding to IP Interface Address of %s\n",
				inet_ntoa(address.sin_addr));
	} else
		address.sin_addr.s_addr = INADDR_ANY;
	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);
	if( bind (newsocket, (void *)&address, sizeof(address)) == -1)
		Sys_Error ("UDP_OpenSocket: bind: %s", strerror(errno));

	return newsocket;
}

void NET_GetLocalAddress (void)
{
	char	buff[MAXHOSTNAMELEN];
	struct sockaddr_in	address;
	int		namelen;

	gethostname(buff, MAXHOSTNAMELEN);
	buff[MAXHOSTNAMELEN-1] = 0;

	NET_StringToAdr (buff, &net_local_adr);

	namelen = sizeof(address);
	if (getsockname (net_socket, (struct sockaddr *)&address, &namelen) == -1)
		Sys_Error ("NET_Init: getsockname:", strerror(errno));
	net_local_adr.port = address.sin_port;

	Con_Printf("IP address %s\n", NET_AdrToString (net_local_adr) );
}

/*
====================
NET_Init
====================
*/
void NET_Init (int port)
{
	//
	// open the single socket to be used for all communications
	//
	net_socket = UDP_OpenSocket (port);

	//
	// init the message buffer
	//
	net_message.maxsize = sizeof(net_message_buffer);
	net_message.data = net_message_buffer;

	//
	// determine my name & address
	//
	NET_GetLocalAddress ();

	Con_Printf("UDP Initialized\n");
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	close (net_socket);
}

