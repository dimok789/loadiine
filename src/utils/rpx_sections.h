#ifndef RPX_SECTIONS_H_
#define RPX_SECTIONS_H_

#include "../common/common.h"

int GetMemorySegment(s_rpx_rpl * rpx_data, unsigned int offset, unsigned int size, unsigned char *buffer);
int GetRpxImports(s_rpx_rpl * rpx_data, char ***rpl_import_list);

#endif // RPX_SECTIONS_H_
