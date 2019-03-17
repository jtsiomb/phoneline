#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include "modem.h"
#include "util.h"
#include "opt.h"

#define DBGPRINT
#define MAX_RING_INTERVAL	8

int init_modem(void)
{
	struct termios term;

	if(!isatty(modem_fd)) {
		perror("init_modem failed");
		return -1;
	}

	if(tcgetattr(modem_fd, &term) == -1) {
		fprintf(stderr, "failed to get terminal attributes: %s\n", strerror(errno));
		return -1;
	}
	cfmakeraw(&term);
	cfsetispeed(&term, B115200);
	cfsetospeed(&term, B115200);
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 0;
	if(tcsetattr(modem_fd, TCSANOW, &term) == -1) {
		fprintf(stderr, "failed to set terminal attributes: %s\n", strerror(errno));
		return -1;
	}

	return modem_cmd(20, "ATQ0;E0;S0=0;#CID=1");
}

int handle_modem(void)
{
	static time_t last_ring;
	static char datestr[5];
	static char timestr[5];
	char *line;
	time_t t;
	struct tm *tm;

	while((line = read_line(modem_fd))) {
		line = clean_line(line);
		if(!line || !*line) continue;

#ifdef DBGPRINT
		printf("DBG modem says: \"%s\"\n", line);
#endif

		if(strcmp(line, "RING") == 0) {
			t = time(0);
			if(t - last_ring > MAX_RING_INTERVAL) {
				tm = localtime(&t);
				printf("[%02d/%02d/%d - %02d:%02d] ring!\n", tm->tm_mday + 1,
						tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min);
				callerid[0] = 0;
				alarm(MAX_RING_INTERVAL);
			}
			last_ring = t;
		} else {
			if(time(0) - last_ring > 30) {
				continue;
			}
		}

		if(memcmp(line, "DATE = ", 7) == 0) {
			strncpy(datestr, line + 7, sizeof datestr - 1);
		} else if(memcmp(line, "TIME = ", 7) == 0) {
			strncpy(timestr, line + 7, sizeof timestr - 1);
		} else if(memcmp(line, "NMBR = ", 7) == 0) {
			char *numstr = clean_line(line + 7);
			if(atol(numstr)) {
				strncpy(callerid, numstr, sizeof callerid - 1);
			}

			alarm(0);
			ring_pending = 1;
			printf("  call %s %s: %s\n", datestr, timestr, callerid[0] ? callerid : "unknown");
		}
	}

	return 0;
}


int modem_cmd(int timeout, const char *fmt, ...)
{
	char *line;
	char buf[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf - 3, fmt, ap);
	va_end(ap);
	strcat(buf, "\r\n");

	/* send initialization string and wait for an answer */
	write(modem_fd, buf, strlen(buf));

	for(;;) {
		fd_set rdset;
		struct timeval tv;

		FD_ZERO(&rdset);
		FD_SET(modem_fd, &rdset);

		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		if(select(modem_fd + 1, &rdset, 0, 0, &tv) <= 0 && errno != EINTR) {
			fprintf(stderr, "failed to get response from modem at %s\n", opt.devfile);
			return -1;
		}

		if(FD_ISSET(modem_fd, &rdset)) {
			while((line = read_line(modem_fd))) {
				if(strcmp(line, "OK\r\n") == 0) {
					return 0;
				}
				if(strcmp(line, "ERROR\r\n") == 0) {
					return -1;
				}
			}
		}
	}
	return 0;
}

void hangup(void)
{
	modem_cmd(10, "ATH0");
	write(modem_fd, "ATA0\r\n", 6);
	sleep(1);
	write(modem_fd, "A", 1);
	sleep(1);
	modem_cmd(10, "AT");
	while(read_line(modem_fd));
}
