#include "../common/common.h"
#include "../common/kernel_defs.h"
#include "../utils/strings.h"

/* our BSS struct */
extern ReducedCosAppXmlInfo cosAppXmlInfoStruct;

/*
 *  This function is a kernel hook function. It is called directly from kernel code at position 0xFFF18558.
 */
void my_PrepareTitle(CosAppXmlInfo *xmlKernelInfo)
{
    /*
     * Setup a DBAT access for our 0xC0800000 area and our 0xBC000000 area which hold our variables like GAME_LAUNCHED and our BSS/rodata section
     */
    register int dbatu0, dbatl0, dbatu1, dbatl1;
    // save the original DBAT value
    asm volatile("mfdbatu %0, 0" : "=r" (dbatu0));
    asm volatile("mfdbatl %0, 0" : "=r" (dbatl0));
    asm volatile("mfdbatu %0, 1" : "=r" (dbatu1));
    asm volatile("mfdbatl %0, 1" : "=r" (dbatl1));
    // write our own DBATs into the array
    asm volatile("mtdbatu 0, %0" : : "r" (0xC0001FFF));
    asm volatile("mtdbatl 0, %0" : : "r" (0x30000012));
    asm volatile("mtdbatu 1, %0" : : "r" (0xB0801FFF));
    asm volatile("mtdbatl 1, %0" : : "r" (0x20800012));
    asm volatile("eieio; isync");


    // check for Mii Maker RPX or Smash Bros RPX when we started (region independent check)
    if(GAME_LAUNCHED && ((strncasecmp("ffl_app.rpx", xmlKernelInfo->rpx_name, FS_MAX_ENTNAME_SIZE) == 0) || (strncasecmp("cross_f.rpx", xmlKernelInfo->rpx_name, FS_MAX_ENTNAME_SIZE) == 0)))
    {
        //! Copy all data from the parsed XML info
        strlcpy(xmlKernelInfo->rpx_name, cosAppXmlInfoStruct.rpx_name, sizeof(cosAppXmlInfoStruct.rpx_name));

        xmlKernelInfo->version_cos_xml = cosAppXmlInfoStruct.version_cos_xml;
        xmlKernelInfo->os_version = cosAppXmlInfoStruct.os_version;
        xmlKernelInfo->title_id = cosAppXmlInfoStruct.title_id;
        xmlKernelInfo->app_type = cosAppXmlInfoStruct.app_type;
        xmlKernelInfo->cmdFlags = cosAppXmlInfoStruct.cmdFlags;
        xmlKernelInfo->max_size = cosAppXmlInfoStruct.max_size;
        xmlKernelInfo->avail_size = cosAppXmlInfoStruct.avail_size;
        xmlKernelInfo->codegen_size = cosAppXmlInfoStruct.codegen_size;
        xmlKernelInfo->codegen_core = cosAppXmlInfoStruct.codegen_core;
        xmlKernelInfo->max_codesize = cosAppXmlInfoStruct.max_codesize;
        xmlKernelInfo->overlay_arena = cosAppXmlInfoStruct.overlay_arena;
        xmlKernelInfo->default_stack0_size = cosAppXmlInfoStruct.default_stack0_size;
        xmlKernelInfo->default_stack1_size = cosAppXmlInfoStruct.default_stack1_size;
        xmlKernelInfo->default_stack2_size = cosAppXmlInfoStruct.default_stack2_size;
        xmlKernelInfo->default_redzone0_size = cosAppXmlInfoStruct.default_redzone0_size;
        xmlKernelInfo->default_redzone1_size = cosAppXmlInfoStruct.default_redzone1_size;
        xmlKernelInfo->default_redzone2_size = cosAppXmlInfoStruct.default_redzone2_size;
        xmlKernelInfo->exception_stack0_size = cosAppXmlInfoStruct.exception_stack0_size;
        xmlKernelInfo->exception_stack1_size = cosAppXmlInfoStruct.exception_stack1_size;
        xmlKernelInfo->exception_stack2_size = cosAppXmlInfoStruct.exception_stack2_size;
        xmlKernelInfo->sdk_version = cosAppXmlInfoStruct.sdk_version;
        xmlKernelInfo->title_version = cosAppXmlInfoStruct.title_version;
    }

    /*
     * Restore original DBAT value
     */
    asm volatile("mfdbatu %0, 0" : "=r" (dbatu0));
    asm volatile("mfdbatl %0, 0" : "=r" (dbatl0));
    asm volatile("mfdbatu %0, 1" : "=r" (dbatu1));
    asm volatile("mfdbatl %0, 1" : "=r" (dbatl1));
    asm volatile("eieio; isync");
}
