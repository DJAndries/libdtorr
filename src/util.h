#ifndef UTIL_H
#define UTIL_H

unsigned int bigend_to_uint(char* buf);

void uint_to_bigend(char* buf, unsigned int val);

unsigned long get_time_ms();

void dsleep(unsigned long ms);

unsigned long calc_piece_length(unsigned long piece_count, unsigned long piece_length, unsigned long total_length, unsigned long index);

#endif