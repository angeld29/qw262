/*
 *  Cygwin console input functions
 *
 *  Copyright (C) 2005 Se7en
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
 *  $Id: sv_cygwin.c,v 1.2 2005/01/31 13:04:21 borisu Exp $
 */

#include <windows.h>

static HANDLE std_input = INVALID_HANDLE_VALUE;
static HANDLE std_output = INVALID_HANDLE_VALUE;

int _kbhit (void)
{
    INPUT_RECORD *input_buf;
    DWORD num, records;
    BOOL rc;
    int i;
	char input_char;

    if (std_input == INVALID_HANDLE_VALUE)
		return 0;

    rc = GetNumberOfConsoleInputEvents (std_input, &num);
    if (!rc || !num)
		return 0;

    input_buf = malloc (sizeof(INPUT_RECORD)*num);
    if (!input_buf)
		return 0;

    rc = PeekConsoleInput (std_input, input_buf, num, &records);

    if (!rc || !records)
    {
		free (input_buf);
		return 0;
    }
    for (i = 0; i < records; ++i)
    {
		if (input_buf[i].EventType != KEY_EVENT)
			continue;
		if (!input_buf[i].Event.KeyEvent.bKeyDown)
			continue;
		input_char = input_buf[i].Event.KeyEvent.uChar.AsciiChar;
		if (input_char && input_char != 27)
		{
			free (input_buf);
			return 1;
		}
    }

    free (input_buf);
    return 0;
}

int _getch (void)
{
    char chr[4]; // for Unicode characters
    DWORD num;
    BOOL rc;

    if (std_input == INVALID_HANDLE_VALUE)
		return (-1);

    rc = ReadConsole (std_input, chr, 1, &num, NULL);
    if (rc && num == 1)
		return (chr[0]);
    else
		return (-1);
}

void putch (int symbol)
{
    char chr;
    DWORD num;

    if (std_output == INVALID_HANDLE_VALUE)
		return;

    chr = symbol;

    WriteConsole (std_output, &chr, 1, &num, NULL);
}

void cygwin_console_init (void)
{
    std_input = GetStdHandle (STD_INPUT_HANDLE);
    std_output = GetStdHandle (STD_OUTPUT_HANDLE);
    SetConsoleMode (std_input, ENABLE_PROCESSED_INPUT);
}
