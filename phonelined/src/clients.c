#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "opt.h"
#include "calldb.h"
#include "util.h"
#include "modem.h"

static void send_call_log(int s, int maxlines);

#define MAX_CLIENTS		64
static int csock[MAX_CLIENTS];
static int num_clients;

int add_client(int s)
{
	if(num_clients >= MAX_CLIENTS) {
		fprintf(stderr, "no space for more clients, dropping\n");
		return -1;
	}
	printf("new client: %d (fd: %d)\n", num_clients, s);
	csock[num_clients++] = s;
	return 0;
}

int *client_sockets(int *count)
{
	int i;

	/* compact client sockets */
	for(i=0; i<num_clients; i++) {
		if(csock[i] == -1) {
			memmove(csock + i, csock + i + 1, num_clients - i - 1);
			num_clients--;
		}
	}

	*count = num_clients;
	return csock;
}

int handle_client(int cid)
{
	int s, maxlines;
	char *line;

	s = csock[cid];

	while((line = read_line(s))) {
		line = clean_line(line);
		if(memcmp(line, "bye", 3) == 0) {
			printf("client %d is leaving\n", cid);
			close(s);
			csock[cid] = -1;
			break;

		} else if(sscanf(line, "log %d", &maxlines) == 1) {
			printf("sending call log (%d lines) to client %d\n", maxlines, cid);
			send_call_log(s, maxlines);

		} else if(memcmp(line, "hangup", 6) == 0) {
			printf("hanging up\n");
			hangup();
		}
	}

	if(last_rdsize == 0) {
		printf("client disconnected\n");
		close(s);
		csock[cid] = -1;
		return -1;
	}
	return 0;
}

void ping_clients(void)
{
	int i;

	for(i=0; i<num_clients; i++) {
		if(csock[i] == -1) continue;

		if(write(csock[i], "ping\n", 5) == -1) {
			printf("client disconnected\n");
			close(csock[i]);
			csock[i] = -1;
		}
	}
}

void notify_ring(const char *telnum)
{
	int i;
	char buf[64];

	sprintf(buf, "ring %s\n", telnum ? telnum : "unknown");

	for(i=0; i<num_clients; i++) {
		if(csock[i] == -1) continue;

		if(write(csock[i], buf, strlen(buf)) == -1) {
			printf("client disconnected\n");
			close(csock[i]);
			csock[i] = -1;
		}
	}
}

static void send_call_log(int s, int maxlines)
{
	int i, len;
	char buf[256];

	for(i=0; i<maxlines; i++) {
		struct call *call = get_call(i);
		if(!call) break;

		len = sprintf(buf, "log %lu:%s\n", call->time, call->number);
		write(s, buf, len);
	}
	write(s, "endlog\n", 7);
}
