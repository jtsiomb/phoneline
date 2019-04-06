#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "util.h"

int read_char(int fd)
{
	static char buf[128];
	static char *next, *end;

	if(next >= end) {
		if((last_rdsize = read(fd, buf, sizeof buf)) <= 0) {
			if(last_rdsize == 0) {
				/* socket closed on the remote side */
				close(fd);
			}
			return -1;
		}
		next = buf;
		end = buf + last_rdsize;
	}

	return *next++;
}

char *read_line(int fd)
{
	static char line[512];
	static int len;
	char *ptr = line + len;
	int c;

	while((c = read_char(fd)) != -1) {
		if(len < sizeof line - 1) {
			*ptr++ = c;
			len++;
		}
		if(c == '\n') {
			*ptr = 0;
			len = 0;
			return line;
		}
	}
	return 0;
}

char *clean_line(char *s)
{
	char *endp;
	while(*s && isspace(*s)) s++;
	endp = s + strlen(s) - 1;
	while(endp >= s && isspace(*endp)) {
		*endp-- = 0;
	}
	return endp < s ? 0 : s;
}
