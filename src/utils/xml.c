#include "../common/fs_defs.h"
#include "../common/kernel_defs.h"
#include "../fs/fs_functions.h"
#include "../menu/menu.h"
#include "utils.h"
#include "strings.h"
#include "logger.h"

#define XML_BUFFER_SIZE         8192

char * XML_GetNodeText(const char *xml_part, const char * nodename, char * output, int output_size)
{
    // create '<' + nodename
    char buffer[strlen(nodename) + 3];
    buffer[0] = '<';
    strlcpy(&buffer[1], nodename, sizeof(buffer));

    const char *start = strcasestr(xml_part, buffer);
    if(!start)
        return 0;

    // find closing tag
    while(*start && *start != '>')
        start++;
    // skip '>'
    if(*start == '>')
        start++;

    // create '</' + nodename
    buffer[0] = '<';
    buffer[1] = '/';
    strlcpy(&buffer[2], nodename, sizeof(buffer));

    // search for end of string
    const char *end = strcasestr(start, buffer);
    if(!end)
        return 0;

    // copy the stuff in between
    int len = 0;
    while(start < end && len < (output_size-1))
    {
        output[len++] = *start++;
    }

    output[len] = 0;
    return output;
}

int LoadXmlParameters(ReducedCosAppXmlInfo * xmlInfo, FSClient *pClient, FSCmdBlock *pCmd, const char *rpx_name, const char *path, int path_index)
{
    //--------------------------------------------------------------------------------------------
    // setup default data
    //--------------------------------------------------------------------------------------------
    memset(xmlInfo, 0, sizeof(ReducedCosAppXmlInfo));
    xmlInfo->version_cos_xml = 18;             // default for most games
    xmlInfo->os_version = 0x000500101000400A;  // default for most games
    xmlInfo->title_id = title_id;              // use mii maker ID
    xmlInfo->app_type = 0x80000000;            // default for most games
    xmlInfo->cmdFlags = 0;                     // default for most games
    strlcpy(xmlInfo->rpx_name, rpx_name, sizeof(xmlInfo->rpx_name));
    xmlInfo->max_size = 0x40000000;            // default for most games
    xmlInfo->avail_size = 0;                   // default for most games
    xmlInfo->codegen_size = 0;                 // default for most games
    xmlInfo->codegen_core = 1;                 // default for most games
    xmlInfo->max_codesize = 0x03000000;        // i think this is the best for most games
    xmlInfo->overlay_arena = 0;                // only very few have that set to 1
    xmlInfo->exception_stack0_size = 0x1000;   // default for most games
    xmlInfo->exception_stack1_size = 0x1000;   // default for most games
    xmlInfo->exception_stack2_size = 0x1000;   // default for most games
    xmlInfo->sdk_version = 20909;              // game dependent, lets take the one from mii maker
    xmlInfo->title_version = 0;                // game dependent, we say its 0
    //--------------------------------------------------------------------------------------------

    int fd = 0;
    char* path_copy = (char*)malloc(FS_MAX_MOUNTPATH_SIZE);
    if (!path_copy)
        return -1;

    char* xmlData = (char*)malloc(XML_BUFFER_SIZE);
    if(!xmlData) {
        free(path_copy);
        return -2;
    }

    char* xmlNodeData = (char*)malloc(XML_BUFFER_SIZE);
    if(!xmlNodeData) {
        free(xmlData);
        free(path_copy);
        return -3;
    }

    memset(xmlData, 0, XML_BUFFER_SIZE);

    // create path
    __os_snprintf(path_copy, FS_MAX_MOUNTPATH_SIZE, "%s/cos.xml", path);

    if (FSOpenFile(pClient, pCmd, path_copy, "r", &fd, FS_RET_ALL_ERROR) == FS_STATUS_OK)
    {
        // read first up to XML_BUFFER_SIZE available bytes and close file
        int result = FSReadFile(pClient, pCmd, xmlData, 0x1, XML_BUFFER_SIZE, fd, 0, FS_RET_ALL_ERROR);
        FSCloseFile(pClient, pCmd, fd, FS_RET_NO_ERROR);
        // lets start parsing
        if(result > 0)
        {
            // ensure 0 termination
            xmlData[XML_BUFFER_SIZE-1] = 0;


            if(XML_GetNodeText(xmlData, "version", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 10);
                xmlInfo->version_cos_xml = value;
            }
            if(XML_GetNodeText(xmlData, "cmdFlags", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 10);
                xmlInfo->cmdFlags = value;
            }
            // always use RPX name from FS
            //if(XML_GetNodeText(xmlData, "argstr", xmlNodeData, XML_BUFFER_SIZE))
            //{
            //    strlcpy(xmlInfo->rpx_name, xmlNodeData, sizeof(xmlInfo->rpx_name));
            //}
            if(XML_GetNodeText(xmlData, "avail_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->avail_size = value;
            }
            if(XML_GetNodeText(xmlData, "codegen_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->codegen_size = value;
            }
            if(XML_GetNodeText(xmlData, "codegen_core", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->codegen_core = value;
            }
            if(XML_GetNodeText(xmlData, "max_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->max_size = value;
            }
            if(XML_GetNodeText(xmlData, "max_codesize", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->max_codesize = value;
            }
            if(XML_GetNodeText(xmlData, "overlay_arena", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->overlay_arena = value;
            }
            if(XML_GetNodeText(xmlData, "default_stack0_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->default_stack0_size = value;
            }
            if(XML_GetNodeText(xmlData, "default_stack1_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->default_stack1_size = value;
            }
            if(XML_GetNodeText(xmlData, "default_stack2_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->default_stack2_size = value;
            }
            if(XML_GetNodeText(xmlData, "default_redzone0_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->default_redzone0_size = value;
            }
            if(XML_GetNodeText(xmlData, "default_redzone1_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->default_redzone1_size = value;
            }
            if(XML_GetNodeText(xmlData, "default_redzone2_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->default_redzone2_size = value;
            }
            if(XML_GetNodeText(xmlData, "exception_stack0_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->exception_stack0_size = value;
            }
            if(XML_GetNodeText(xmlData, "exception_stack1_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->exception_stack0_size = value;
            }
            if(XML_GetNodeText(xmlData, "exception_stack2_size", xmlNodeData, XML_BUFFER_SIZE))
            {
                unsigned int value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->exception_stack0_size = value;
            }
        }
    }

    // reset buffer
    memset(xmlData, 0, XML_BUFFER_SIZE);

    // create path
    __os_snprintf(path_copy, FS_MAX_MOUNTPATH_SIZE, "%s/app.xml", path);

    if (FSOpenFile(pClient, pCmd, path_copy, "r", &fd, FS_RET_ALL_ERROR) == FS_STATUS_OK)
    {
        // read first up to XML_BUFFER_SIZE available bytes and close file
        int result = FSReadFile(pClient, pCmd, xmlData, 0x1, XML_BUFFER_SIZE, fd, 0, FS_RET_ALL_ERROR);
        FSCloseFile(pClient, pCmd, fd, FS_RET_NO_ERROR);
        // lets start parsing
        if(result > 0)
        {
            // ensure 0 termination
            xmlData[XML_BUFFER_SIZE-1] = 0;

            //--------------------------------------------------------------------------------------------
            // version tag is still unknown where it is used
            //--------------------------------------------------------------------------------------------
            if(XML_GetNodeText(xmlData, "os_version", xmlNodeData, XML_BUFFER_SIZE))
            {
                uint64_t value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->os_version = value;
            }
            if(XML_GetNodeText(xmlData, "title_id", xmlNodeData, XML_BUFFER_SIZE))
            {
                uint64_t value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->title_id = value;

            }
            if(XML_GetNodeText(xmlData, "title_version", xmlNodeData, XML_BUFFER_SIZE))
            {
                uint32_t value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->title_version = value;
            }
            if(XML_GetNodeText(xmlData, "sdk_version", xmlNodeData, XML_BUFFER_SIZE))
            {
                uint32_t value = strtoll(xmlNodeData, 0, 10);
                xmlInfo->sdk_version = value;
            }
            if(XML_GetNodeText(xmlData, "app_type", xmlNodeData, XML_BUFFER_SIZE))
            {
                uint32_t value = strtoll(xmlNodeData, 0, 16);
                xmlInfo->app_type = value;
            }
            //--------------------------------------------------------------------------------------------
            // group_id tag is still unknown where it is used
            //--------------------------------------------------------------------------------------------
        }
    }

    free(xmlData);
    free(xmlNodeData);
    free(path_copy);

    return 0;
}
