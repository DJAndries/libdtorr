#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void default_log_handler(int log_level, char* msg) {
  char* txt_level = "";
  FILE* stream = stdout;
  switch (log_level) {
    case LOG_LEVEL_ERROR:
      txt_level = "ERROR";
      stream = stderr;
      break;
    case LOG_LEVEL_WARN:
      txt_level = "WARN";
      break;
    case LOG_LEVEL_INFO:
      txt_level = "INFO";
      break;
    case LOG_LEVEL_DEBUG:
      txt_level = "DEBUG";
      break;
  }
  fprintf(stream, "dtorr: [%s]: %s\n", txt_level, msg);
}

void dlog(dtorr_config* config, int log_level, char* message, ...) {
  char fmt_message[512];
  va_list argptr;

  void (*log_handler)(int, char*) = config->log_handler;
  if (log_handler == 0) {
    log_handler = &default_log_handler;
  }

  if (log_level > config->log_level) {
    return;
  }

  va_start(argptr, message);
  vsprintf(fmt_message, message, argptr);
  va_end(argptr);

  log_handler(log_level, fmt_message);
}
