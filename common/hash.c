#include "bothdefs.h"
#include "hash.h"

unsigned char Hash(char *str)
{
	unsigned h = 0;
	unsigned l = 1;
	char	 ch;
	while (*str) {
		ch = *str;
		if (ch >= 'A' && ch <= 'Z')
			ch += 32;
		h = h*5 + ch * l++;
		str++;
	}
	return h % HT_SIZE;
}
