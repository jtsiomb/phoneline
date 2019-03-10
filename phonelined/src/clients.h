#ifndef CLIENTS_H_
#define CLIENTS_H_

int add_client(int s);
int *client_sockets(int *count);
int handle_client(int cid);
void ping_clients(void);
void notify_ring(const char *telnum);

#endif	/* CLIENTS_H_ */
