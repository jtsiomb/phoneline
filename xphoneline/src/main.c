#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <X11/Xlib.h>

#define DEF_PORT	1111

static int handle_xevent(XEvent *ev);
static int parse_args(int argc, char **argv);
static void print_usage(const char *argv0);

static Display *dpy;

static char *hostname;
static int port = DEF_PORT;

int main(int argc, char **argv)
{
	int s, xs;
	struct hostent *host;
	struct sockaddr_in addr;

	if(parse_args(argc, argv) == -1) {
		return 1;
	}
	if(!hostname) {
		fprintf(stderr, "you must specify the phonelined server hostname\n");
		return 1;
	}

	if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1){
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
	fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return 1;
	}
	xs = ConnectionNumber(dpy);

	for(;;) {
		int res, maxfd;
		fd_set rdset;

		FD_ZERO(&rdset);
		FD_SET(s, &rdset);
		FD_SET(xs, &rdset);
		maxfd = s > xs ? s : xs;

		if((res = select(maxfd + 1, &rdset, 0, 0, 0)) == -1 && errno != EINTR) {
			break;
		}
		if(res <= 0) continue;

		if(FD_ISSET(s, &rdset)) {
			char buf[128];
			while((res = read(s, buf, sizeof buf)) > 0) {
				/* TODO handle phonelined traffic */
			}
			if(res == 0) goto done;
		}
		if(FD_ISSET(xs, &rdset)) {
			while(XPending(dpy)) {
				XEvent ev;
				XNextEvent(dpy, &ev);
				if(handle_xevent(&ev) == -1) {
					goto done;
				}
			}
		}
	}

done:
	close(s);
	XCloseDisplay(dpy);
	return 0;
}

static int handle_xevent(XEvent *ev)
{
	return 0;	/* TODO */
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
	printf(" -h: print usage information and exit\n");
}
