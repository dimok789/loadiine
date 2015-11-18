#ifndef __XML_H_
#define __XML_H_

#include "../common/kernel_defs.h"

char * XML_GetNodeText(const char *xml_part, const char * nodename, char * output, int output_size);
int LoadXmlParameters(ReducedCosAppXmlInfo * xmlInfo, FSClient *pClient, FSCmdBlock *pCmd, const char *rpx_name, const char *path, int path_index);

#endif // __XML_H_
