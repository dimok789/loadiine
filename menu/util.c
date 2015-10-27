#include "util.h"

static inline int toupper(int c) {
    return (c >= 'a' && c <= 'z') ? (c - 0x20) : c;
}

int strlen(const char* str) {
    int i = 0;
    while (str[i])
        i++;
    return i;
}

/* not used yet, maybe later
int strlcpy(char *s1, const char *s2, unsigned int max_size)
{
    if(!s1 || !s2 || !max_size)
        return 0;

    int len = 0;
    while(s2[len] && (len < (max_size-1)))
    {
        s1[len] = s2[len];
        len++;
    }
    s1[len] = 0;
    return len;
}
*/
int strcasecmp(const char *s1, const char *s2) {
    if(!s1 || !s2) {
        return -1;
    }

    while(*s1 && *s2)
    {
        int diff = toupper(*s1) - toupper(*s2);
        if(diff != 0) {
            return diff;
        }

        s1++;
        s2++;
    }
    int diff = toupper(*s1) - toupper(*s2);
    if(diff != 0) {
        return diff;
    }
    return 0;
}

static void swap(void *e1, void *e2, unsigned int length)
{
    unsigned char tmp;
    unsigned char *a = (unsigned char *)e1;
    unsigned char *b = (unsigned char *)e2;

    while(length--)
    {
        tmp = *a;
        *a = *b;
        *b = tmp;
        a++;
        b++;
    }
}

void qsort(void *ptr, unsigned int count, unsigned int size, int (*compare)(const void*,const void*))
{
    if((count <= 1) || (size == 0)) {
        return;
    }

    unsigned char *begin = (unsigned char*)ptr;
    unsigned char *end = begin + size * count;
     // the pivot is improvable but for now it's ok, we don't need fastest sort algorithm as speed is not an issue ;-)
    unsigned char *pivot = begin;

    unsigned char *left = pivot + size;
    unsigned char *right = end;

    while ( left < right )
    {
        if (compare(left, pivot) <= 0 )  {
            left += size;
        }
        else {
            right -= size;

            if(left != right)
                swap(left, right, size);
        }
    }

    left -= size;

    if(begin != left)
        swap(begin, left, size);

    int left_num = (left - begin) / size;
    if(left_num > 1)
        qsort(begin, left_num, size, compare);

    int right_num = (end - right) / size;
    if(right_num > 1)
        qsort(right, right_num, size, compare);
}
