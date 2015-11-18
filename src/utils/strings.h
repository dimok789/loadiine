#ifndef __STRINGS_H_
#define __STRINGS_H_

#include "../common/types.h"

static inline int toupper(int c) {
    return (c >= 'a' && c <= 'z') ? (c - 0x20) : c;
}

void* memcpy(void *dst, const void *src, unsigned int len);
void* memset(void *dst, int val, unsigned int len);
int memcmp (const void * ptr1, const void * ptr2, unsigned int num);

/* string functions */
int strncasecmp(const char *s1, const char *s2, unsigned int max_len);
int strncmp(const char *s1, const char *s2, unsigned int max_len);
int strncpy(char *dst, const char *src, unsigned int max_size);
int strlcpy(char *s1, const char *s2, unsigned int max_size);
int strnlen(const char* str, unsigned int max_size);
int strlen(const char* str);
const char *strcasestr(const char *str, const char *pattern);
int64_t strtoll(const char *str, char **end, int base);

/* functions from OS, don't put standard functions here without any prefix marking it as an OS function */
extern int __os_snprintf(char* s, int n, const char * format, ...);

#endif // __STRINGS_H_
