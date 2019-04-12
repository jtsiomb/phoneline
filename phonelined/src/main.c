#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pwd.h>
#include "opt.h"
#include "modem.h"
#include "clients.h"
#include "util.h"
#include "calldb.h"

#undef DBGPRINT

#define LOGFILE	"/var/log/phonelined.log"

static void cleanup(void);
static void daemonize(void);
static void sighandler(int s);

static int lis = -1;

int main(int argc, char **argv)
{
	static const int reuse = 1;
	struct sockaddr_in sa;

	init_options();
	load_config(geteuid() == 0 ? "/etc/phonelined.conf" : "phonelined.conf");
	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	signal(SIGINT, sighandler);
	signal(SIGABRT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGSEGV, sighandler);
	signal(SIGILL, sighandler);
	signal(SIGALRM, sighandler);

	atexit(cleanup);

	if((modem_fd = open(opt.devfile, O_RDWR)) == -1) {
		fprintf(stderr, "failed to open modem device %s: %s\n", opt.devfile, strerror(errno));
		return 1;
	}

	/* open and initialize modem */
	if(init_modem() == -1) {
		fprintf(stderr, "failed to initialize modem\n");
		close(modem_fd);
		return 1;
	}

	/* create listening socket */
	if((lis = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("failed to create listening socket");
		close(modem_fd);
		return 1;
	}
	setsockopt(lis, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
	fcntl(lis, F_SETFL, fcntl(lis, F_GETFL) | O_NONBLOCK);
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(opt.port);
	if(bind(lis, (struct sockaddr*)&sa, sizeof sa) == -1) {
		fprintf(stderr, "failed to bind socket to port %d: %s\n", opt.port, strerror(errno));
		close(modem_fd);
		return 1;
	}
	listen(lis, 32);

	load_calldb();

	/* daemonize and drop priviledges if applicable */
	if(opt.daemonize) {
		daemonize();
	}

	printf("phonelined started monitoring the phone line\n");
	fflush(stdout);

	for(;;) {
		int maxfd, res;
		int *cfd;
		int i, nclients = 0;
		fd_set rdset;
		struct timeval timeout;

		if(ring_pending) {
			char *number = callerid[0] ? callerid : 0;
			add_call(time(0), number);
			notify_ring(number);
			callerid[0] = 0;
			ring_pending = 0;
		}

		FD_ZERO(&rdset);
		FD_SET(modem_fd, &rdset);
		FD_SET(lis, &rdset);
		maxfd = modem_fd > lis ? modem_fd : lis;

		cfd = client_sockets(&nclients);
		for(i=0; i<nclients; i++) {
			FD_SET(cfd[i], &rdset);
			if(cfd[i] > maxfd) maxfd = cfd[i];
		}

		timeout.tv_sec = 120;
		timeout.tv_usec = 0;
		if((res = select(maxfd + 1, &rdset, 0, 0, &timeout)) <= 0) {
			if(res == 0 || errno != EINTR) {
				/* timed out or caught an error, ping clients to drop any
				 * disconnected ones, and and write any pending call logs
				 */
				ping_clients();
				sync_calldb();
			}
			continue;
		}

		/* we had some activity, figure out what, and handle it */

		if(FD_ISSET(modem_fd, &rdset)) {
			if(handle_modem() == -1) {
				break;
			}
		}
		if(FD_ISSET(lis, &rdset)) {
			int s;

			if((s = accept(lis, 0, 0)) == -1) {
				perror("failed to accept incoming connection");
			} else {
				fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
				add_client(s);
			}
		}
		for(i=0; i<nclients; i++) {
			if(FD_ISSET(cfd[i], &rdset)) {
#ifdef DBGPRINT
				printf("DBG handle_client %d\n", i);
#endif
				handle_client(i);
			}
		}
	}

	return 0;
}

static void cleanup(void)
{
	int i, nclients;
	int *csock;

	if(lis >= 0) {
		csock = client_sockets(&nclients);
		for(i=0; i<nclients; i++) {
			close(csock[i]);
		}
		close(lis);
	}
	if(modem_fd >= 0) close(modem_fd);

	sync_calldb();
}

static void daemonize(void)
{
	int pid;

	chdir("/");

	close(0);
	close(1);
	close(2);

	open("/dev/zero", O_RDONLY);		/* fd 0 */
	open(LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	dup(1);	/* dup 1 -> 2 */

	setvbuf(stdout, 0, _IOLBF, 0);
	setvbuf(stderr, 0, _IONBF, 0);

	/* release controlling terminal */
	if((pid = fork()) == -1) {
		perror("failed to fork");
		exit(1);
	} else if(pid) {
		exit(0);
	}
	setsid();	/* create new session */

	/* drop priviledges, the modem is already open at this point */
	if(geteuid() == 0) {
		struct passwd *pw = getpwnam(opt.runas);
		if(!pw || setuid(pw->pw_uid) == -1) {
			fprintf(stderr, "failed to drop priviledges, continuing as root\n");
		}
	}
}

static void sighandler(int s)
{
	signal(s, sighandler);

	if(s == SIGALRM) {
		ring_pending = 1;
	} else {
		exit(0);
	}
}
