#ifndef _LOADER_H_
#define _LOADER_H_

/* This are the real functions that are used for our purpose */
extern void LiCheckAndHandleInterrupts(void);
extern int Loader_SysCallGetProcessIndex(int procId);
extern int LiLoadAsync(const char *filename, unsigned int address, unsigned int address_size, int fileOffset, int fileType, int processIndex, int minus1);
extern int LiWaitIopComplete(int unknown_syscall_arg_r3, int * remaining_bytes);
extern int LiWaitIopCompleteWithInterrupts(int unknown_syscall_arg_r3, int * remaining_bytes);

/* This are just addresses to the real functions which we only need as reference */
extern int addr_LiBounceOneChunk(const char * filename, int fileType, int procId, int * hunkBytes, int fileOffset, int bufferNumber, int * dst_address);
extern int addr_LiWaitOneChunk(unsigned int * iRemainingBytes, const char *filename, int fileType);

#endif /* _LOADER_H_ */
