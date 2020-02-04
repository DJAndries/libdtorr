#ifndef UTIL_H
#define UTIL_H

unsigned int bigend_to_uint(char* buf);

void uint_to_bigend(char* buf, unsigned int val);

unsigned long get_time_ms();

#endif