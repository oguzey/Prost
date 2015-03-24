#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#define DEBUG
#ifdef DEBUG

#define log(value, ...) printf(value, __VA_ARGS__)
#define log_int(value)  printf("[%s] %s = %d\n", __func__, #value, value)
#define log_hexint(value)  printf("[%s] %s = %08x\n", __func__, #value, value)
#define log_hexlong(value)  printf("[%s] %s = %016llx\n", __func__, #value, value)

#else

#define log(value,...)
#define log_int(value)
#define log_text(value,...)
#define log_hexint(value)
#define log_hexlong(value)

#endif
#endif // LOG_H
