#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "opt.h"

static void print_usage(const char *argv0);

void init_options(void)
{
	opt.devfile = "/dev/ttyS0";
	opt.daemonize = geteuid() == 0;
	opt.runas = "nobody";
	opt.port = 1111;
}

int load_config(const char *fname)
{
	int nline = 0;
	FILE *fp;
	char buf[128];
	char *line, *endp, *valstr;

	if(!(fp = fopen(fname, "r"))) {
		return -1;
	}

	while(fgets(buf, sizeof buf, fp)) {
		nline++;
		line = buf;
		while(*line && isspace(*line)) line++;
		if((endp = strrchr(line, '#'))) {
			*endp = 0;
		}
		endp = line + strlen(line) - 1;
		while(endp > line && isspace(*endp)) {
			*endp-- = 0;
		}

		if(endp <= line) continue;

		if(!(valstr = strchr(line, '=')) || !valstr[1]) {
			fprintf(stderr, "%s:%d invalid config line: %s\n", fname, nline, line);
			continue;
		}
		endp = valstr - 1;
		*valstr++ = 0;

		while(endp > line && isspace(*endp)) {
			*endp-- = 0;
		}
		if(endp <= line) {
			fprintf(stderr, "%s:%d invalid config line: %s\n", fname, nline, line);
			continue;
		}

		while(*valstr && isspace(*valstr)) valstr++;
		if(!*valstr) {
			fprintf(stderr, "%s:%d invalid config line: %s\n", fname, nline, line);
			continue;
		}

		if(strcmp(line, "device") == 0) {
			if(!(opt.devfile = malloc(strlen(valstr) + 1))) {
				perror("failed to allocate memory");
				abort();
			}
			strcpy(opt.devfile, valstr);

		} else if(strcmp(line, "user") == 0) {
			if(!(opt.runas = malloc(strlen(valstr) + 1))) {
				perror("failed to allocate memory");
				abort();
			}
			strcpy(opt.runas, valstr);

		} else if(strcmp(line, "daemonize") == 0) {
			if(strcmp(valstr, "true") == 0 || strcmp(valstr, "yes") == 0 ||
					strcmp(valstr, "1") == 0) {
				opt.daemonize = 1;
			} else if(strcmp(valstr, "false") == 0 || strcmp(valstr, "no") == 0 ||
					strcmp(valstr, "0") == 0) {
				opt.daemonize = 0;
			} else {
				fprintf(stderr, "%s:%d invalid value for option %s: %s\n", fname, nline,
						line, valstr);
			}

		} else if(strcmp(line, "port") == 0) {
			int p = atoi(valstr);
			if(p <= 0 || p > 65535) {
				fprintf(stderr, "%s:%d invalid port number: %s\n", fname, nline, valstr);
			}

		} else {
			fprintf(stderr, "%s:%d invalid option: %s\n", fname, nline, line);
			continue;
		}
	}

	fclose(fp);
	return 0;
}

int parse_args(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-device") == 0 || strcmp(argv[i], "-D") == 0) {
				opt.devfile = argv[++i];
			} else if(strcmp(argv[i], "-user") == 0 || strcmp(argv[i], "-u") == 0) {
				opt.runas = argv[++i];
			} else if(strcmp(argv[i], "-debug") == 0 || strcmp(argv[i], "-d") == 0) {
				opt.daemonize = 0;
			} else if(strcmp(argv[i], "-port") == 0 || strcmp(argv[i], "-p") == 0) {
				if((opt.port = atoi(argv[++i])) <= 0 || opt.port > 65535) {
					fprintf(stderr, "invalid port number: %s\n", argv[i]);
					return -1;
				}
			} else if(strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
				print_usage(argv[0]);
				exit(0);
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				print_usage(argv[0]);
				return -1;
			}
		} else {
			fprintf(stderr, "unexpected argument: %s\n", argv[i]);
			print_usage(argv[0]);
			return -1;
		}
	}

	return 0;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [options]\n", argv0);
	printf("Options:\n");
	printf(" -D,-device <devfile>: set modem device file\n");
	printf(" -p,-port <port>: listening port for clients\n");
	printf(" -u,-user <username>: drop priviledges and run as this user\n");
	printf(" -d,-debug: debug mode, don't daemonize\n");
	printf(" -h,-help: print usage information and exit\n");
}
