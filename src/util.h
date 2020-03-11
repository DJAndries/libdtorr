#ifndef UTIL_H
#define UTIL_H

#ifdef _WIN32
#define SPEC_LLU "%I64u"
#define SPEC_LLD "%I64d"
#else
#define SPEC_LLU "%llu"
#define SPEC_LLD "%lld"
#endif

unsigned int bigend_to_uint(char* buf);

void uint_to_bigend(char* buf, unsigned int val);

unsigned long long get_time_ms();

void dsleep(unsigned long long ms);

unsigned long long calc_piece_length(unsigned long long piece_count, unsigned long long piece_length, unsigned long long total_length, unsigned long long index);

#endif
