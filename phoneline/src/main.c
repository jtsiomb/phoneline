#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "util.h"

#define DEF_PORT	1111
/* maximum timeout about 8-9 minutes */
#define MAX_TIMEOUT	512

static int open_conn(void);
static int sendcmd(const char *cmd);
static void handle_input(void);
static int handle_resp(int fd);
static int parse_args(int argc, char **argv);
static void print_usage(const char *argv0);
static void print_cmd_help(void);

static enum { INTERACTIVE, PRINT_LOG, HANGUP } mode;

static int s = -1;
static char *hostname;
static int port = DEF_PORT;

int main(int argc, char **argv)
{
	int timeout;

	if(parse_args(argc, argv) == -1) {
		return 1;
	}
	if(!hostname) {
		fprintf(stderr, "you must specify the server hostname\n");
		return 1;
	}

	if(open_conn() == -1) {
		return 1;
	}

	switch(mode) {
	case PRINT_LOG:
		sendcmd("log 10\n");
		break;

	case HANGUP:
		sendcmd("hangup\n");
		return 0;

	default:
		break;
	}

	for(;;) {
		int res, maxfd = 0;
		fd_set rdset;
		struct timeval tv;

		FD_ZERO(&rdset);
		FD_SET(0, &rdset);

		if(s >= 0) {
			FD_SET(s, &rdset);
			maxfd = s;
		} else {
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
		}

		if((res = select(maxfd + 1, &rdset, 0, 0, s >= 0 ? 0 : &tv)) == -1 && errno != EINTR) {
			close(s);
			fprintf(stderr, "select: lost connection to server.\n");
			s = -1;
			timeout = 4;
			continue;
		}

		if(res > 0) {
			if(FD_ISSET(0, &rdset)) {
				handle_input();
			}
			if(FD_ISSET(s, &rdset)) {
				if(handle_resp(s) == -1) {
					fprintf(stderr, "handle_resp: lost connection to server.\n");
					s = -1;
					timeout = 4;
					continue;
				}
			}
		}

		if(s < 0) {
			printf("trying to reconnect to %s:%d ... ", hostname, port);
			fflush(stdout);
			if(open_conn() == 0) {
				printf("done (fd: %d).\n", s);
			} else {
				printf("failed.\n");
			}

			if(timeout < MAX_TIMEOUT) {
				timeout *= 2;
			}
		}
	}

	close(s);
	return 0;
}

static int open_conn(void)
{
	struct hostent *host;
	struct sockaddr_in addr;

	if(s >= 0) return 0;	/* already connected */

	if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("failed to create socket");
		return -1;
	}
	if(!(host = gethostbyname(hostname))) {
		fprintf(stderr, "failed to resolve %s: %s\n", hostname, hstrerror(h_errno));
		close(s);
		s = -1;
		return -1;
	}
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = *(struct in_addr*)host->h_addr;
	if(connect(s, (struct sockaddr*)&addr, sizeof addr) == -1) {
		fprintf(stderr, "failed to connect to %s:%d (%s): %s\n", hostname, port,
				inet_ntoa(addr.sin_addr), strerror(errno));
		close(s);
		s = -1;
		return -1;
	}
	fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
	return 0;
}

static int sendcmd(const char *cmd)
{
	if(s < 0) {
		fprintf(stderr, "Can't send command, not connected!\n");
		return -1;
	}
	return write(s, cmd, strlen(cmd)) > 0 ? 0 : -1;
}

static void handle_input(void)
{
	char buf[512], *cmd;
	int sz;

	while((sz = read(0, buf, sizeof buf - 1)) <= 0 && errno == EINTR);
	if(sz <= 0) exit(0);

	buf[sz] = 0;

	if(!(cmd = clean_line(buf)) || !*cmd) {
		return;
	}

	if(strcmp(cmd, "log") == 0) {
		sendcmd("log 10\n");
	} else if(strcmp(cmd, "hangup") == 0 || strcmp(cmd, "hup") == 0) {
		sendcmd("hangup\n");
	} else if(strcmp(cmd, "quit") == 0 || strcmp(cmd, "bye") == 0) {
		exit(0);
	} else if(strcmp(cmd, "help") == 0) {
		print_cmd_help();
	} else {
		fprintf(stderr, "unknown command: %s\n", cmd);
		print_cmd_help();
	}
}

static int handle_resp(int fd)
{
	char *line;

	while((line = read_line(fd))) {
		if(memcmp(line, "ring ", 5) == 0) {
			printf("Phone ringing: %s\n", clean_line(line + 5));

		} else if(memcmp(line, "log ", 4) == 0) {
			char *sep = strchr(line, ':');
			if(sep) {
				time_t t = atol(line + 4);
				char *tmstr = ctime(&t);
				tmstr[strlen(tmstr) - 1] = 0;
				printf("%s - %s\n", tmstr, clean_line(sep + 1));
			}

		} else if(memcmp(line, "endlog", 6) == 0) {
			if(mode != INTERACTIVE) {
				exit(0);
			}

		} else if(memcmp(line, "ping", 4) == 0) {
			write(fd, "pong\n", 5);

		} else {
			fprintf(stderr, "unexpected message: %s\n", line);
		}
	}

	return errno == EWOULDBLOCK || errno == EAGAIN ? 0 : -1;
}

static int parse_args(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(argv[i][2] == 0) {
				switch(argv[i][1]) {
				case 'p':
					if((port = atoi(argv[++i])) <= 0 || port > 65535) {
						fprintf(stderr, "-p must be followed by a valid port number\n");
						return -1;
					}
					break;

				case 'l':
					mode = PRINT_LOG;
					break;

				case 'H':
					mode = HANGUP;
					break;

				case 'h':
					print_usage(argv[0]);
					exit(0);

				default:
					fprintf(stderr, "invalid option: %s\n", argv[i]);
					print_usage(argv[0]);
					return -1;
				}
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				print_usage(argv[0]);
				return -1;
			}

		} else {
			if(hostname) {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				print_usage(argv[0]);
				return -1;
			}
			hostname = argv[i];
		}
	}

	return 0;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [options] <host>\n", argv0);
	printf("Options:\n");
	printf(" -p <port>: specify the server port number (default: %d)\n", DEF_PORT);
	printf(" -l: retrieve and print call log\n");
	printf(" -H: hangup call (caution might incur charges to the caller!)\n");
	printf(" -h: print usage information and exit\n");
}

static void print_cmd_help(void)
{
	printf("Available commands:\n");
	printf("log           - print call log\n");
	printf("hangup or hup - hangup call (caution might incur charges to the caller!)\n");
	printf("quit or bye   - disconnect and exit\n");
	printf("help          - print command help\n");
}
