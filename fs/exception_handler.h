#ifndef __EXCEPTION_HANDLER_H_
#define __EXCEPTION_HANDLER_H_

#include "../common/fs_defs.h"

/* Exception handling */
#define OS_EXCEPTION_MODE_SYSTEM               0
#define OS_EXCEPTION_MODE_THREAD               1
#define OS_EXCEPTION_MODE_GLOBAL               2
#define OS_EXCEPTION_MODE_THREAD_ALL_CORES     3
#define OS_EXCEPTION_MODE_GLOBAL_ALL_CORES     4

#define OS_EXCEPTION_SYSTEM_RESET         0
#define OS_EXCEPTION_MACHINE_CHECK        1
#define OS_EXCEPTION_DSI                  2
#define OS_EXCEPTION_ISI                  3
#define OS_EXCEPTION_EXTERNAL_INTERRUPT   4
#define OS_EXCEPTION_ALIGNMENT            5
#define OS_EXCEPTION_PROGRAM              6
#define OS_EXCEPTION_FLOATING_POINT       7
#define OS_EXCEPTION_DECREMENTER          8
#define OS_EXCEPTION_SYSTEM_CALL          9
#define OS_EXCEPTION_TRACE                10
#define OS_EXCEPTION_PERFORMANCE_MONITOR  11
#define OS_EXCEPTION_BREAKPOINT           12
#define OS_EXCEPTION_SYSTEM_INTERRUPT     13
#define OS_EXCEPTION_ICI                  14
#define OS_EXCEPTION_MAX                  (OS_EXCEPTION_ICI + 1)

/* Exceptions */
typedef struct OSContext
{
  /* OSContext identifier */
  uint32_t tag1;
  uint32_t tag2;

  /* GPRs */
  uint32_t gpr[32];

  /* Special registers */
  uint32_t cr;
  uint32_t lr;
  uint32_t ctr;
  uint32_t xer;

  /* Initial PC and MSR */
  uint32_t srr0;
  uint32_t srr1;

  /* Only valid during DSI exception */
  uint32_t exception_specific0;
  uint32_t exception_specific1;

  /* There is actually a lot more here but we don't need the rest*/
} OSContext;

typedef unsigned char (*OSExceptionCallback)(OSContext* interruptedContext);
typedef uint8_t OSExceptionMode;
typedef uint8_t OSExceptionType;

extern OSExceptionCallback OSSetExceptionCallbackEx(OSExceptionMode exceptionMode, OSExceptionType exceptionType, OSExceptionCallback newCallback);
extern void OSFatal(const char* msg);
extern int __os_snprintf(char* s, int n, const char * format, ...);

void setup_os_exceptions(void);

#endif
