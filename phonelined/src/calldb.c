#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "calldb.h"

#define CALLDB_FILE	"/var/cache/phoneline.calls"

#define MAX_NUM_CALLS	256
static struct call callbuf[MAX_NUM_CALLS];
static int cnext;
static int write_pending;

void add_call(time_t time, const char *numstr)
{
	callbuf[cnext].time = time;
	strcpy(callbuf[cnext].number, numstr);
	cnext = (cnext + 1) & (MAX_NUM_CALLS - 1);
	write_pending = 1;
}

struct call *get_call(int idx)
{
	if(idx >= MAX_NUM_CALLS) return 0;

	idx = (cnext + MAX_NUM_CALLS - idx - 1) & (MAX_NUM_CALLS - 1);

	return *callbuf[idx].number ? callbuf + idx : 0;
}

int load_calldb(void)
{
	int i, fd;

	if((fd = open(CALLDB_FILE, O_RDONLY)) == -1) {
		fprintf(stderr, "failed to open call database " CALLDB_FILE ": %s\n", strerror(errno));
		return -1;
	}

	if(read(fd, callbuf, sizeof callbuf) < sizeof callbuf) {
		fprintf(stderr, "call database " CALLDB_FILE " invalid or corrupted\n");
		close(fd);
		return -1;
	}

	for(i=0; i<MAX_NUM_CALLS; i++) {
		if(callbuf[i].number[0] == 0) {
			break;
		}
	}
	cnext = i & (MAX_NUM_CALLS - 1);
	write_pending = 0;

	close(fd);
	return 0;
}

void write_calldb(void)
{
	int i, fd, count, left = MAX_NUM_CALLS;

	if((fd = open(CALLDB_FILE, O_WRONLY | O_CREAT | O_TRUNC)) == -1) {
		fprintf(stderr, "failed to open call database " CALLDB_FILE ": %s\n", strerror(errno));
		return;
	}

	for(i=cnext; i<MAX_NUM_CALLS; i++) {
		if(callbuf[i].number[0]) {
			count = MAX_NUM_CALLS - i;
			write(fd, callbuf + i, count * sizeof *callbuf);
			left -= count;
			break;
		}
	}

	write(fd, callbuf, left * sizeof *callbuf);
	close(fd);
	write_pending = 0;
}

void sync_calldb(void)
{
	if(write_pending) {
		write_calldb();
		write_pending = 0;
	}
}
