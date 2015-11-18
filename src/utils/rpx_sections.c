#include "../common/common.h"
#include "../menu/menu.h"
#include "strings.h"
#include "utils.h"
#include "zlib.h"

/* RPX stuff */
#define RPX_SH_STRNDX_OFFSET            0x0032
#define RPX_SHT_START                   0x0040
#define RPX_SHT_ENTRY_SIZE              0x28

#define RPX_SHDR_FLAGS_OFFSET           0x08
#define RPX_SHDR_OFFSET_OFFSET          0x10
#define RPX_SHDR_SIZE_OFFSET            0x14

#define RPX_SHDR_ZLIB_FLAG              0x08000000

int GetMemorySegment(s_rpx_rpl * rpx_data, unsigned int offset, unsigned int size, unsigned char *buffer)
{
    s_mem_area *mem_area = rpx_data->area;
    int mem_area_addr_start = mem_area->address;
    int mem_area_addr_end   = mem_area_addr_start + mem_area->size;
    int mem_area_offset     = rpx_data->offset;

    unsigned int buffer_position = 0;

    // Replace rpx/rpl data
    for (unsigned int position = 0; position < rpx_data->size; position++)
    {
        if ((mem_area_addr_start + mem_area_offset) >= mem_area_addr_end) // TODO: maybe >, not >=
        {
            mem_area            = mem_area->next;
            if(!mem_area) {
                return buffer_position;
            }
            mem_area_addr_start = mem_area->address;
            mem_area_addr_end   = mem_area_addr_start + mem_area->size;
            mem_area_offset     = 0;
        }
        if(position >= offset) {
            buffer[buffer_position] = *(unsigned char *)(mem_area_addr_start + mem_area_offset);
            buffer_position++;
        }
        if(buffer_position >= size) {
            return buffer_position;
        }
        mem_area_offset++;
    }

    return 0;
}

int GetRpxImports(s_rpx_rpl * rpx_data, char ***rpl_import_list)
{
    unsigned char *buffer = (unsigned char *)malloc(0x1000);
    if(!buffer) {
        return 0;
    }

    // get the header information of the RPX
    if(!GetMemorySegment(rpx_data, 0, 0x1000, buffer))
    {
        free(buffer);
        return 0;
    }
    // Who needs error checks...
    // Get section number
    int shstrndx = *(unsigned short*)(&buffer[RPX_SH_STRNDX_OFFSET]);
    // Get section offset
    int section_offset = *(unsigned int*)(&buffer[RPX_SHT_START + (shstrndx * RPX_SHT_ENTRY_SIZE) + RPX_SHDR_OFFSET_OFFSET]);
    // Get section size
    int section_size = *(unsigned int*)(&buffer[RPX_SHT_START + (shstrndx * RPX_SHT_ENTRY_SIZE) + RPX_SHDR_SIZE_OFFSET]);
    // Get section flags
    int section_flags = *(unsigned int*)(&buffer[RPX_SHT_START + (shstrndx * RPX_SHT_ENTRY_SIZE) + RPX_SHDR_FLAGS_OFFSET]);

    // Align read to 64 for SD access (section offset already aligned to 64 @ elf/rpx?!)
    int section_offset_aligned = (section_offset / 64) * 64;
    int section_alignment = section_offset - section_offset_aligned;
    int section_size_aligned = ((section_alignment + section_size) / 64) * 64 + 64;

    unsigned char *section_data = (unsigned char *)malloc(section_size_aligned);
    if(!section_data) {
        free(buffer);
        return 0;
    }

    // get the header information of the RPX
    if(!GetMemorySegment(rpx_data, section_offset_aligned, section_size_aligned, section_data))
    {
        free(buffer);
        free(section_data);
        return 0;
    }

    unsigned char *section_data_inflated = NULL;
    char *uncompressed_data = NULL;
    int uncompressed_data_size = 0;

    //Check if inflate is needed (ZLIB flag)
    if (section_flags & RPX_SHDR_ZLIB_FLAG)
    {
        // Section is compressed, inflate
        int section_size_inflated = *(unsigned int*)(&section_data[section_alignment]);
        section_data_inflated = (unsigned char *)malloc(section_size_inflated);
        if(!section_data_inflated)
        {
            free(buffer);
            free(section_data);
            return 0;
        }

        unsigned int zlib_handle;
        OSDynLoad_Acquire("zlib125", &zlib_handle);

        /* Zlib functions */
        int(*ZinflateInit_)(z_streamp strm, const char *version, int stream_size);
        int(*Zinflate)(z_streamp strm, int flush);
        int(*ZinflateEnd)(z_streamp strm);

        OSDynLoad_FindExport(zlib_handle, 0, "inflateInit_", &ZinflateInit_);
        OSDynLoad_FindExport(zlib_handle, 0, "inflate", &Zinflate);
        OSDynLoad_FindExport(zlib_handle, 0, "inflateEnd", &ZinflateEnd);

        int ret = 0;
        z_stream s;
        memset(&s, 0, sizeof(s));

        s.zalloc = Z_NULL;
        s.zfree = Z_NULL;
        s.opaque = Z_NULL;

        ret = ZinflateInit_(&s, ZLIB_VERSION, sizeof(s));
        if (ret != Z_OK)
        {
            free(buffer);
            free(section_data);
            free(section_data_inflated);
            return 0;
        }

        s.avail_in = section_size - 0x04;
        s.next_in = section_data + section_alignment + 0x04;

        s.avail_out = section_size_inflated;
        s.next_out = section_data_inflated;

        ret = Zinflate(&s, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END)
        {
            // Inflate failed
            free(buffer);
            free(section_data);
            free(section_data_inflated);
            return 0;
        }

        ZinflateEnd(&s);
        uncompressed_data = (char *) section_data_inflated;
        uncompressed_data_size = section_size_inflated;
    } else {
        uncompressed_data = (char *) section_data;
        uncompressed_data_size = section_size;
    }

    // Parse imports
    int offset = 0;
    int rpl_count = 0;
    char **list = 0;
    do
    {
        if (strncmp(&uncompressed_data[offset+1], ".fimport_", 9) == 0)
        {
            char **new_list = (char**)malloc(sizeof(char*) * (rpl_count + 1));
            if(!new_list) {
                break;
            }
            else if(!list) {
                // first entry
                list = new_list;
            }
            else {
                // reallocate the list
                memcpy(new_list, list, sizeof(char*) * rpl_count);
                free(list);
                list = new_list;
            }

            // Match, save import in list
            int len = strlen(&uncompressed_data[offset+1+9]);
            list[rpl_count] = (char *)malloc(len + 1);

            if(list[rpl_count]) {
                memcpy(list[rpl_count], &uncompressed_data[offset+1+9], len + 1);
                rpl_count++;
            }
        }
        offset++;
        while (uncompressed_data[offset] != 0) {
            offset++;
        }
    } while(offset + 1 < uncompressed_data_size);

    // Free section data
    if (section_data_inflated) {
        free(section_data_inflated);
    }
    free(section_data);
    free(buffer);

    // set pointer
    if(rpl_count > 0)
    {
        // if we want to copy the list, then copy it
        if(rpl_import_list) {
            *rpl_import_list = list;
        }
        // otherwise free the memory of the list
        else if(list) {
            int j;
            for(j = 0; j < rpl_count; j++)
                if(list[j])
                    free(list[j]);

            free(list);
        }
    }

    return rpl_count;
}
