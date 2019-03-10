#ifndef CALLDB_H_
#define CALLDB_H_

#include <time.h>

struct call {
	time_t time;
	char number[64];
};

void add_call(time_t time, const char *numstr);
struct call *get_call(int idx);

int load_calldb(void);
void write_calldb(void);
void sync_calldb(void);

#endif	/* CALLDB_H_ */
