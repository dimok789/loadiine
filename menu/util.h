#ifndef __UTIL_H_
#define __UTIL_H_

int strcasecmp(const char *s1, const char *s2);
int strlen(const char* str);
//int strlcpy(char *s1, const char *s2, unsigned int max_size);
void qsort(void *ptr, unsigned int count, unsigned int size, int (*compare)(const void*,const void*));

#endif
