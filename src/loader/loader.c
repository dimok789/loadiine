#include "loader.h"
#include "../common/common.h"
#include "../common/loader_defs.h"
#include "../utils/strings.h"

#define REPLACE_LIBOUNCEONECHUNK      0

// Comment (Dimok):
// we don't need to replace this function as i never seen it fail actually
// it doesn't even fail when fileOffset is above the actually RPX/RPL fileSize
// just in case I added this implementation if we ever want to override it
#if REPLACE_LIBOUNCEONECHUNK
static int LiBounceOneChunk(const char * filename, int fileType, int procId, int * hunkBytes, int fileOffset, int bufferNumber, int * dst_address)
{
    unsigned int address;
    loader_globals_t *loader_globals = (loader_globals_t*)(0xEFE19D00);

    // handle interrupts
    LiCheckAndHandleInterrupts();

    // bufferNumber = 1 is 0xF6000000 and bufferNumber = 2 is 0xF6400000
    if(bufferNumber == 1)
        address = 0xF6000000;
    else
        address = 0xF6400000;

    // if global RPX/RPL name is not copied yet do it now
    if(loader_globals->sgLoadName[0] == 0) {
        strncpy(loader_globals->sgLoadName, filename, sizeof(loader_globals->sgLoadName)-1);
        loader_globals->sgLoadName[sizeof(loader_globals->sgLoadName)-1] = 0;
    }
    // set all remaining global variables
    loader_globals->sgFileOffset = fileOffset;
    loader_globals->sgBufferNumber = bufferNumber;
    loader_globals->sgFileType = fileType;
    loader_globals->sgProcId = procId;
    loader_globals->sgBounceError = 0;

    // get process index for the current process ID
    int processIndex = Loader_SysCallGetProcessIndex(procId);
    // give the IOSU a signal to start loading data into the address asynchronously
    int result = LiLoadAsync(filename, address, 0x400000, fileOffset, fileType, processIndex, -1);

    // our little hack
    // If we are in Smash Bros app or Mii Maker
    // just in case something with loading asynchron the data from IOSU fails we ignore the error (never seen it happen)
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (GAME_LAUNCHED == 1))
    {
        result = 0;
    }

    // check the result
    if(result == 0) {
        // if no error occurs, the data hunkBytes are set to maximum possible in one bounce
        *hunkBytes = 0x400000;
        // the output for address is also set at this position
        if(dst_address)
            *dst_address = address;

        // enable global flag that buffer is now loading data
        loader_globals->sgIsLoadingBuffer = 1;

        // handle interrupts again
        LiCheckAndHandleInterrupts();
    }
    else {
        // TODO comment by Dimok:
        // here comes a lot of error handling which we skip
    }

    return result;
}
#endif

// This function is called every time after LiBounceOneChunk.
// It waits for the asynchronous call of LiLoadAsync for the IOSU to fill data to the RPX/RPL address
// and return the still remaining bytes to load.
// We override it and replace the loaded date from LiLoadAsync with our data and our remaining bytes to load.
static int LiWaitOneChunk(unsigned int * iRemainingBytes, const char *filename, int fileType)
{
    unsigned int result;
    register int core_id;
    int remaining_bytes = 0;
    // pointer to global variables of the loader
    loader_globals_t *loader_globals = (loader_globals_t*)(0xEFE19D00);

    // get the current core
    asm volatile("mfspr %0, 0x3EF" : "=r" (core_id));

    // Comment (Dimok):
    // time measurement at this position for logger  -> we don't need it right now except maybe for debugging
    //unsigned long long systemTime1 = Loader_GetSystemTime();

    // get the offset of per core global variable for dynload initialized (just a simple address + (core_id * 4))
    unsigned int gDynloadInitialized = *(volatile unsigned int*)(0xEFE13C3C + (core_id << 2));

    // the data loading was started in LiBounceOneChunk() and here it waits for IOSU to finish copy the data
    if(gDynloadInitialized != 0) {
        result = LiWaitIopCompleteWithInterrupts(0x2160EC0, &remaining_bytes);

    }
    else {
        result = LiWaitIopComplete(0x2160EC0, &remaining_bytes);
    }


    // Comment (Dimok):
    // time measurement at this position for logger -> we don't need it right now except maybe for debugging
    //unsigned long long systemTime2 = Loader_GetSystemTime();

    //------------------------------------------------------------------------------------------------------------------
    // Start of our function intrusion:
    // After IOSU is done writing the data into the 0xF6000000/0xF6400000 address,
    // we overwrite it with our data before setting the global flag for IsLoadingBuffer to 0
    // Do this only if we are in the game that was launched by our method
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (GAME_LAUNCHED == 1))
    {
        s_rpx_rpl *rpl_struct = (s_rpx_rpl*)(RPX_RPL_ARRAY);

        do
        {
            // the argument fileType = 0 is RPX, fileType = 1 is RPL (inverse to our is_rpx)
            if((!rpl_struct->is_rpx) != fileType)
                continue;

            int found = 1;

            // if we load RPX then the filename can't be checked as it is the Mii Maker or Smash Bros RPX name
            // there skip the filename check for RPX
            if(fileType == 1)
            {
                int len = strnlen(filename, 0x1000);
                int len2 = strnlen(rpl_struct->name, 0x1000);
                if ((len != len2) && (len != (len2 - 4)))
                    continue;

                if(strncasecmp(filename, rpl_struct->name, len) != 0) {
                    found = 0;
                }
            }

            if (found)
            {
                unsigned int load_address = (loader_globals->sgBufferNumber == 1) ? 0xF6000000 : 0xF6400000;

                // set our game RPX loaded variable for use in FS system
                if(fileType == 0)
                    GAME_RPX_LOADED = 1;

                remaining_bytes = rpl_struct->size - loader_globals->sgFileOffset;
                if (remaining_bytes > 0x400000)
                    // truncate size
                    remaining_bytes = 0x400000;

                s_mem_area *mem_area    = rpl_struct->area;
                int mem_area_addr_start = mem_area->address;
                int mem_area_addr_end   = mem_area_addr_start + mem_area->size;
                int mem_area_offset     = rpl_struct->offset;

                // Replace rpx/rpl data
                for (int i = 0; i < (remaining_bytes / 4); i++)
                {
                    if ((mem_area_addr_start + mem_area_offset) >= mem_area_addr_end) // TODO: maybe >, not >=
                    {
                        mem_area            = mem_area->next;
                        mem_area_addr_start = mem_area->address;
                        mem_area_addr_end   = mem_area_addr_start + mem_area->size;
                        mem_area_offset     = 0;
                    }
                    *(volatile unsigned int *)(load_address + (i * 4)) = *(volatile unsigned int *)(mem_area_addr_start + mem_area_offset);
                    mem_area_offset += 4;
                }

                rpl_struct->area = mem_area;
                rpl_struct->offset = mem_area_offset;
                // set result to 0 -> "everything OK"
                result = 0;
                break;
            }
        }
        while((rpl_struct = rpl_struct->next) != 0);
    }
    // check if the game was left back to system menu after a game was launched by our method and reset our flags for game launch
    else if (*(volatile unsigned int*)0xEFE00000 != RPX_CHECK_NAME && (GAME_LAUNCHED == 1) && (GAME_RPX_LOADED == 1))
    {
        GAME_RPX_LOADED = 0;
        GAME_LAUNCHED = 0;
        RPX_CHECK_NAME = 0xDEADBEAF;
    }

    // end of our little intrusion into this function
    //------------------------------------------------------------------------------------------------------------------

    // set the result to the global bounce error variable
    loader_globals->sgBounceError = result;
    // disable global flag that buffer is still loaded by IOSU
    loader_globals->sgIsLoadingBuffer = 0;

    // check result for errors
    if(result == 0) {
        // the remaining size is set globally and in stack variable only
        // if a pointer was passed to this function
        if(iRemainingBytes) {
            loader_globals->sgGotBytes = remaining_bytes;
            *iRemainingBytes = remaining_bytes;
        }
        // Comment (Dimok):
        // calculate time difference and print it on logging how long the wait for asynchronous data load took
        // something like (systemTime2 - systemTime1) * constant / bus speed, did not look deeper into it as we don't need that crap
    }
    else {
        // Comment (Dimok):
        // a lot of error handling here. depending on error code sometimes calls Loader_Panic() -> we don't make errors so we can skip that part ;-P
    }
    return result;
}

// this macro generates the the magic entry
#define MAKE_MAGIC(x, call_type) { x, addr_ ## x, call_type }
// A kind of magic used to store :
// - replace instruction address
// - replace instruction
const struct magic_t {
    const void * repl_func;           // our replacement function which is called
    const void * repl_addr;           // address where to place the jump to the our function
    const unsigned int call_type;     // call type, e.g. 0x48000000 for branch and 0x48000001 for branch link
} loader_methods[] __attribute__((section(".loader_magic"))) = {
#if REPLACE_LIBOUNCEONECHUNK
    MAKE_MAGIC(LiBounceOneChunk,                            0x48000002),      // simple branch to our function as we replace it completely and don't want LR to be replaced by bl
#endif
    MAKE_MAGIC(LiWaitOneChunk,                              0x48000002),      // simple branch to our function as we replace it completely and don't want LR to be replaced by bl
};
