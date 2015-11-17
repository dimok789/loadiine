#ifndef __UTILS_H_
#define __UTILS_H_

#include "../common/types.h"

#define FlushBlock(addr)   asm volatile("dcbf %0, %1\n"                                \
                                        "icbi %0, %1\n"                                \
                                        "sync\n"                                       \
                                        "eieio\n"                                      \
                                        "isync\n"                                      \
                                        :                                              \
                                        :"r"(0), "r"(((addr) & ~31))                   \
                                        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"     \
                                        );


/* Alloc */
extern void *(* const MEMAllocFromDefaultHeapEx)(int size, int align);
extern void *(* const MEMAllocFromDefaultHeap)(int size);
extern void *(* const MEMFreeToDefaultHeap)(void *ptr);

#define memalign (*MEMAllocFromDefaultHeapEx)
#define malloc (*MEMAllocFromDefaultHeap)
#define free (*MEMFreeToDefaultHeap)

extern const uint64_t title_id;

int  fs_mount_sd(int sock, void* pClient, void* pCmd);
void qsort(void *ptr, unsigned int count, unsigned int size, int (*compare)(const void*,const void*));

#endif // __UTILS_H_
