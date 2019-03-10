#ifndef UTIL_H_
#define UTIL_H_

int last_rdsize;

int read_char(int fd);
char *read_line(int fd);
char *clean_line(char *s);

#endif	/* UTIL_H_ */
