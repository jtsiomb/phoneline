#ifndef MODEM_H_
#define MODEM_H_

int modem_fd;
volatile int ring_pending;
char callerid[64];

int init_modem(void);
int handle_modem(void);
int modem_cmd(int timeout, const char *fmt, ...);
void hangup(void);

#endif	/* MODEM_H_ */
