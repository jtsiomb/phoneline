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

static int handle_resp(int fd);
static int parse_args(int argc, char **argv);
static void print_usage(const char *argv0);

static enum { WAIT_RING, PRINT_LOG, HANGUP } mode;
static char *hostname;
static int port = DEF_PORT;

int main(int argc, char **argv)
{
	int s;
	struct hostent *host;
	struct sockaddr_in addr;

	if(parse_args(argc, argv) == -1) {
		return 1;
	}
	if(!hostname) {
		fprintf(stderr, "you must specify the server hostname\n");
		return 1;
	}

	if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("failed to create socket");
		return 1;
	}
	if(!(host = gethostbyname(hostname))) {
		fprintf(stderr, "failed to resolve %s: %s\n", hostname, hstrerror(h_errno));
		return 1;
	}
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = *(struct in_addr*)host->h_addr;
	if(connect(s, (struct sockaddr*)&addr, sizeof addr) == -1) {
		fprintf(stderr, "failed to connect to %s:%d (%s): %s\n", hostname, port,
				inet_ntoa(addr.sin_addr), strerror(errno));
		return 1;
	}

	switch(mode) {
	case PRINT_LOG:
		write(s, "log 10\n", 7);
		break;

	case HANGUP:
		write(s, "hangup\n", 7);
		return 0;

	default:
		break;
	}

	fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);

	for(;;) {
		int res;
		fd_set rdset;

		FD_ZERO(&rdset);
		FD_SET(s, &rdset);

		if((res = select(s + 1, &rdset, 0, 0, 0)) == -1 && errno != EINTR) {
			break;
		}

		if(res > 0) {
			if(FD_ISSET(s, &rdset)) {
				if(handle_resp(s) == -1) {
					break;
				}
			}
		}
	}

	close(s);
	return 0;
}

static int handle_resp(int fd)
{
	char *line;

	while((line = read_line(fd))) {
		if(memcmp(line, "ring ", 5) == 0) {
			printf("Phone ringing: %s\n", line + 5);

		} else if(memcmp(line, "log ", 4) == 0) {
			char *sep = strchr(line, ':');
			if(sep) {
				time_t t = atol(line + 4);
				char *tmstr = ctime(&t);
				tmstr[strlen(tmstr) - 1] = 0;
				printf("%s - %s\n", tmstr, clean_line(sep + 1));
			}

		} else if(memcmp(line, "endlog", 6) == 0) {
			return -1;

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
