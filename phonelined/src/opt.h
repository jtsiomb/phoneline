#ifndef OPT_H_
#define OPT_H_

struct options {
	char *devfile;
	char *runas;
	int daemonize;
	int port;
} opt;

void init_options(void);
int load_config(const char *fname);
int parse_args(int argc, char **argv);

#endif	/* OPT_H_ */
