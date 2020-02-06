#ifndef UTIL_H
#define UTIL_H

unsigned int bigend_to_uint(char* buf);

void uint_to_bigend(char* buf, unsigned int val);

unsigned long get_time_ms();

void dsleep(unsigned long ms);

long bitfield_interest_index(char* ours, char* theirs, unsigned long len, char random);

#endif