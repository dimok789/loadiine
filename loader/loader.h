#ifndef _LOADER_H_
#define _LOADER_H_

#include "../common/types.h"

/* Instructions addresses in loader to replace by a "bl"(jump) to our replacement function */
extern void addr_LOADER_Start(void);
extern void addr_LOADER_Entry(void);
extern void addr_LOADER_Prep(void);
extern void addr_LiLoadRPLBasics_in_1_load(void);
extern void addr_LiSetupOneRPL_after(void);
extern void addr_GetNextBounce_1(void);
extern void addr_GetNextBounce_2(void);
extern void addr_LiRefillBounceBufferForReading_af_getbounce(void);

#endif /* _LOADER_H_ */
