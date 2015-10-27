#include "loader.h"
#include "../common/common.h"

// dbcf and icbi flush/invalidate 0x20 bytes but we can't be sure that our 4 bytes are all in the same block, so call it for first and last byte
#define FlushBlock(addr)   asm volatile("dcbf %0, %1\n"                                \
                                        "icbi %0, %1\n"                                \
                                        "sync\n"                                       \
                                        "eieio\n"                                      \
                                        "isync\n"                                      \
                                        :                                              \
                                        :"r"(0), "r"(((addr) & ~31))                   \
                                        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"     \
                                        );

// Instruction address after LiWaitOneChunk
#define LI_WAIT_ONE_CHUNK1_AFTER1_ADDR          (0x01009120)
#define LI_WAIT_ONE_CHUNK1_AFTER2_ADDR          (0x01009124)
#define LI_WAIT_ONE_CHUNK1_AFTER1_ORIG_INSTR    0x7c7f1b79 // mr. r31, r3
#define LI_WAIT_ONE_CHUNK1_AFTER2_ORIG_INSTR    0x418201b4 // beq loc_10092d8
#define LI_WAIT_ONE_CHUNK1_AFTER1_NEW_INSTR     0x3be00000 // li r31, 0
#define LI_WAIT_ONE_CHUNK1_AFTER2_NEW_INSTR     0x480001b4 // b loc_10092d8

#define LI_WAIT_ONE_CHUNK2_AFTER1_ADDR          (0x01009300)
#define LI_WAIT_ONE_CHUNK2_AFTER2_ADDR          (0x01009304)
#define LI_WAIT_ONE_CHUNK2_AFTER1_ORIG_INSTR    0x7c7f1b79 // mr. r31, r3
#define LI_WAIT_ONE_CHUNK2_AFTER2_ORIG_INSTR    0x40820078 // bne loc_100937c
#define LI_WAIT_ONE_CHUNK2_AFTER1_NEW_INSTR     0x3be00000 // li r31, 0
#define LI_WAIT_ONE_CHUNK2_AFTER2_NEW_INSTR     0x60000000 // nop

#define LI_WAIT_ONE_CHUNK3_AFTER1_ADDR          (0x0100b6e8)
#define LI_WAIT_ONE_CHUNK3_AFTER2_ADDR          (0x0100b6ec)
#define LI_WAIT_ONE_CHUNK3_AFTER1_ORIG_INSTR    0x7c7f1b79 // mr. r31, r3
#define LI_WAIT_ONE_CHUNK3_AFTER2_ORIG_INSTR    0x408200c0 // bne loc_100b7ac
#define LI_WAIT_ONE_CHUNK3_AFTER1_NEW_INSTR     0x3be00000 // li r31, 0
#define LI_WAIT_ONE_CHUNK3_AFTER2_NEW_INSTR     (0x48000001 | (((unsigned int)GetNextBounce_af_LiWaitOneChunk) - LI_WAIT_ONE_CHUNK3_AFTER2_ADDR))   // replace with jump to our function dynamically


static inline int toupper(int c) {
    return (c >= 'a' && c <= 'z') ? (c - 0x20) : c;
}

static void GetNextBounce_af_LiWaitOneChunk(void);

/* LOADER_Start ****************************************************************
 * - instruction address = 0x01003e60
 * - original instruction = 0x5586103a : "slwi r6, r12, 2"
 * - entry point of the loader
 */
void LOADER_Start(void)
{
    // Save registers
    int r4, r6, r12;
    asm volatile("mr %0, 4\n"
        "mr %1, 6\n"
        "mr %2, 12\n"
        :"=r"(r4), "=r"(r6), "=r"(r12)
        :
        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );

    // Reset flags
//    loader_debug_t* loader = (loader_debug_t *)DATA_ADDR;
//    loader[0].tag = 1;
//    loader[0].data = 0;
//    loader[1].tag = 0;
//    loader[1].data = 0;

    BOUNCE_FLAG_ADDR = 0;         // Bounce flag off
    IS_ACTIVE_ADDR = 0;           // replacement off
    MEM_OFFSET = 0;               // RPX/RPL load offset
    MEM_AREA = 0;                 // RPX/RPL current memory area
    MEM_PART = 0;                 // RPX/RPL part to fill (0=0xF6000000-0xF6400000, 1=0xF6400000-0xF6800000)
    RPL_REPLACE_ADDR = 0;         // Reset
    RPL_ENTRY_INDEX_ADDR = 0;     // Reset
    IS_LOADING_RPX_ADDR = 1;      // Set RPX loading

    // Disable fs
    if ((*(volatile unsigned int *)0xEFE00000 != RPX_CHECK_NAME) && (GAME_RPX_LOADED == 1))
    {
        GAME_RPX_LOADED = 0;
        GAME_LAUNCHED = 0;
    }

    // return properly
    asm volatile("mr 4, %0\n"
        "mr 6, %1\n"
        "mr 12, %2\n"
        :
        :"r"(r4), "r"(r6), "r"(r12)
        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );
    asm volatile("slwi 6, 12, 2\n");
}

/* LOADER_Entry ****************************************************************
 * - instruction address = 0x01002938
 * - original instruction = 0x835e0000 : "lwz r26, 0(r30)"
 */
void LOADER_Entry(void)
{
    // Save registers
    int r30;
    asm volatile("mr %0, 30\n"
        :"=r"(r30)
        :
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "12"
    );

    // Do our stuff
    RPL_REPLACE_ADDR = 0;      // Reset
    IS_LOADING_RPX_ADDR = 0;   // Set RPL loading

    // Logs
//    loader_debug_t * loader = (loader_debug_t *)DATA_ADDR;
//    while(loader->tag != 0)
//        loader++;
//    loader[0].tag = 2;
//    loader[0].data = *(volatile unsigned int *)0xEFE00000;
//    loader[1].tag = 0;

    // return properly
    asm volatile("mr 30, %0\n"
        "lwz 26, 0(30)\n"
        :
        :"r"(r30)
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "12"
    );
}

/* LOADER_Prep *****************************************************************
 * - instruction address = 0X0100A024
 * - original instruction = 0x3f00fff9 : "lis r24, -7"
 */
void LOADER_Prep(void)
{
    // Save registers
    int r29;
    asm volatile("mr %0, 29\n"
        :"=r"(r29)
        :
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );

    // Logs
//    loader_debug_t * loader = (loader_debug_t *)DATA_ADDR;
//    while(loader->tag != 0)
//        loader++;
//    loader[0].tag = 3;
//    loader[0].data = *(volatile unsigned int *)0xEFE00000;
//    loader[1].tag = 0;

    // Reset
    IS_ACTIVE_ADDR = 0; // Set as inactive

    // If we are in Smash Bros app or Mii Maker
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (GAME_LAUNCHED == 1))
    {
        // Check if it is an rpl we want, look for rpl name
        char* rpl_name = (char*)(*(volatile unsigned int *)(r29 + 0x08));
        int len = 0;
        while (rpl_name[len])
            len++;

        s_rpx_rpl *rpl_struct = (s_rpx_rpl*)(RPX_RPL_ARRAY);

        do
        {
            if(rpl_struct->is_rpx)
                continue;

            int len2 = 0;
            while(rpl_struct->name[len2])
                len2++;

            if ((len != len2) && (len != (len2 - 4)))
                continue;

            int found = 1;
            for (int x = 0; x < len; x++)
            {
                if (toupper(rpl_struct->name[x]) != toupper(rpl_name[x]))
                {
                    found = 0;
                    break;
                }
            }

            if (found)
            {
                // Set rpl has to be added, and save entry index
                RPL_REPLACE_ADDR = 1;
                RPL_ENTRY_INDEX_ADDR = (unsigned int)rpl_struct;

                // Patch the loader instruction after LiWaitOneChunk to say : yes it's ok I have data =)
                *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK1_AFTER1_ADDR)) = LI_WAIT_ONE_CHUNK1_AFTER1_NEW_INSTR;
                *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK1_AFTER2_ADDR)) = LI_WAIT_ONE_CHUNK1_AFTER2_NEW_INSTR;

                FlushBlock(LI_WAIT_ONE_CHUNK1_AFTER1_ADDR);
                FlushBlock(LI_WAIT_ONE_CHUNK1_AFTER2_ADDR);
                break;
            }
        }
        while((rpl_struct = rpl_struct->next) != 0);
    }

    // return properly
    asm volatile("lis 24, -7\n"
        "lis 23, -0x1020\n"
        "lwzu 4, 0x1000(23)\n"
        "mr 29, %0\n"
        :
        :"r"(r29)
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "12"
    );
}

/* LiLoadRPLBasics_bef_LiWaitOneChunk ******************************************
 * - instruction address = 0x01009118
 * - original instruction = 0x809d0010 : "lwz r4, 0x10(r29)"
 */
void LiLoadRPLBasics_bef_LiWaitOneChunk(void)
{
    // save registers
    int r29, r3;
    asm("mr %0, 29\n"
        "mr %1, 3\n"
        :"=r"(r29), "=r"(r3)
        :
        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );

    // If we are in Smash Bros app
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (GAME_LAUNCHED == 1))
    {
        s_rpx_rpl *entry = 0;

        if (RPL_REPLACE_ADDR == 1)
        {
            entry = (s_rpx_rpl *)(RPL_ENTRY_INDEX_ADDR);
        }
        else if (IS_LOADING_RPX_ADDR == 1)
        {
            entry = (s_rpx_rpl*)(RPX_RPL_ARRAY);

            while(entry)
            {
                if(entry->is_rpx)
                    break;

                entry = entry->next;
            }
        }

        if (entry)
        {
            // Get rpx/rpl entry
            int size = entry->size;
            if (size > 0x400000)
                size = 0x400000;
            // save stack pointer of RPX/RPL size for later use in LiLoadRPLBasics_in_1_load
            *(volatile unsigned int*)(r3) = size;
            RPX_SIZE_POINTER_1 = r3;
        }
    }

    // return properly
    asm("mr 29, %0\n"
        "lwz 4, 0x10(29)\n"
        :
        :"r"(r29)
        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );
}

/* LiLoadRPLBasics_in_1_load ***************************************************
 * - instruction address = 0x010092E4
 * - original instruction = 0x7EE3BB78 : "mr r3, r23"
 * - The first portion of the rpx is retrieve here, so we replace the first part of rpx here
 */
void LiLoadRPLBasics_in_1_load(void)
{
    // save registers
    int r23, r29;
    asm volatile(
        "mr %0, 23\n"
        "mr %1, 29\n"
        :"=r"(r23), "=r"(r29)
        :
        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );

    // If we are in Smash Bros app
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (GAME_LAUNCHED == 1))
    {
        // Check if it is an rpx, look for rpx name, if it is rpl, it is already marked
        int is_rpx = 0;
        s_rpx_rpl * rpx_rpl_entry = 0;

        // Check if rpl is already marked
        if (RPL_REPLACE_ADDR == 1)
        {
            rpx_rpl_entry = (s_rpx_rpl *) (RPL_ENTRY_INDEX_ADDR);

            // Restore original instruction after LiWaitOneChunk
            *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK1_AFTER1_ADDR)) = LI_WAIT_ONE_CHUNK1_AFTER1_ORIG_INSTR;
            *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK1_AFTER2_ADDR)) = LI_WAIT_ONE_CHUNK1_AFTER2_ORIG_INSTR;

            // Set new instructions
            *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK2_AFTER1_ADDR)) = LI_WAIT_ONE_CHUNK2_AFTER1_NEW_INSTR;
            *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK2_AFTER2_ADDR)) = LI_WAIT_ONE_CHUNK2_AFTER2_NEW_INSTR;

            FlushBlock(LI_WAIT_ONE_CHUNK1_AFTER1_ADDR);
            FlushBlock(LI_WAIT_ONE_CHUNK1_AFTER2_ADDR);
            FlushBlock(LI_WAIT_ONE_CHUNK2_AFTER1_ADDR);
            FlushBlock(LI_WAIT_ONE_CHUNK2_AFTER2_ADDR);
        }
        else
        {
            for (int i = 0x100; i < 0x4000; i++)
            {
                int val = *(volatile unsigned int *)(0xF6000000 + i);
                if (val == 0x2e727078) // ".rpx"
                {
                    is_rpx = 1;
                    rpx_rpl_entry = (s_rpx_rpl*)(RPX_RPL_ARRAY);
                    while(rpx_rpl_entry)
                    {
                        if(rpx_rpl_entry->is_rpx)
                            break;

                        rpx_rpl_entry = rpx_rpl_entry->next;
                    }
                    break;
                }
            }
        }

        // Copy RPX/RPL
        if (rpx_rpl_entry)
        {
            // Copy rpx
            s_mem_area *mem_area    = rpx_rpl_entry->area;
            int mem_area_addr_start = mem_area->address;
            int mem_area_addr_end   = mem_area_addr_start + mem_area->size;
            int mem_area_offset     = rpx_rpl_entry->offset;
            int size                = rpx_rpl_entry->size;
            if (size >= 0x400000) // TODO: put only > not >=
            {
                // truncate size
                size = 0x400000;

                // Patch the loader instruction after LiWaitOneChunk (in GetNextBounce)
                *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK3_AFTER1_ADDR)) = LI_WAIT_ONE_CHUNK3_AFTER1_NEW_INSTR;
                *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK3_AFTER2_ADDR)) = LI_WAIT_ONE_CHUNK3_AFTER2_NEW_INSTR;

                FlushBlock(LI_WAIT_ONE_CHUNK3_AFTER1_ADDR);
                FlushBlock(LI_WAIT_ONE_CHUNK3_AFTER2_ADDR);
            }
            else
            {
                RPL_REPLACE_ADDR = 0;
            }

            // Replace rpx/rpl data
            for (int i = 0; i < (size / 4); i++)
            {
                if ((mem_area_addr_start + mem_area_offset) >= mem_area_addr_end) // TODO: maybe >, not >=
                {
                    mem_area            = mem_area->next;
                    mem_area_addr_start = mem_area->address;
                    mem_area_addr_end   = mem_area_addr_start + mem_area->size;
                    mem_area_offset     = 0;
                }
                *(volatile unsigned int *)(0xF6000000 + i * 4) = *(volatile unsigned int *)(mem_area_addr_start + mem_area_offset);
                mem_area_offset += 4;
            }

            // Reduce size and set offset
            MEM_SIZE    = rpx_rpl_entry->size - size;
            MEM_AREA    = (int)mem_area;
            MEM_OFFSET  = mem_area_offset;
            MEM_PART    = 1;

            // get the stack pointer of the upper calling function and set the correct size directly into stack variables
            unsigned int rpx_size_ptr = RPX_SIZE_POINTER_1;
            *(volatile unsigned int *)rpx_size_ptr = size;
            *(volatile unsigned int*)(r29 + 0x20) = size;

            // If rpx/rpl size is less than 0x400000 bytes, we need to stop here
            if (is_rpx)
            {
                // Set rpx name as active for FS to replace file from sd card
                GAME_RPX_LOADED = 1;
            }

            // Replacement ON
            IS_ACTIVE_ADDR = 1;
        }
    }

    // return properly
    asm("mr 3, %0\n"
        :
        :"r"(r23)
        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );
}

/* GetNextBounce_bef_LiWaitOneChunk ********************************************
 * - instruction address = 0x0100B6E0
 * - original instruction = 0x91010008 : "stw r8, 0x18+var_10(r1)"
 */
unsigned int GetNextBounce_bef_LiWaitOneChunk(unsigned int rpx_size_ptr)
{
    // save the stack pointer of the above function for later use after LiWaitOneChunk is complete
    RPX_SIZE_POINTER_2 = rpx_size_ptr;
    *(volatile unsigned int*)rpx_size_ptr = 0;
    return rpx_size_ptr;
}

/* GetNextBounce_af_LiWaitOneChunk ********************************************
 * This function is called dynamically when RPL/RPX >= 0x400000 are loaded.
 * This function is also patched dynamically and is not called for unknown RPLs/RPXs.
 * - instruction address = 0x0100B6EC
 * - original instruction = 0x408200C0 : "bne loc_100B7AC"
 */
static void GetNextBounce_af_LiWaitOneChunk(void)
{
    // If it is active
    int is_active = IS_ACTIVE_ADDR;
    if (is_active)
    {
        unsigned int rpx_size_ptr = RPX_SIZE_POINTER_2;
        // Check if we need to adjust the read size and offset
        int size = MEM_SIZE;
        if (size > 0x400000)
            size = 0x400000;

        // get stack pointer stored before calling LiWaitOneChunk and store the remaining size in it
        *(volatile unsigned int *)(rpx_size_ptr) = size;
        BOUNCE_FLAG_ADDR = 1; // Bounce flag on
    }
}

/* LiRefillBounceBufferForReading_af_getbounce *********************************
 * - instruction address = 0x0100BA28
 * - original instruction = 0x2C030000 : "cmpwi r3, 0"
 */
void LiRefillBounceBufferForReading_af_getbounce(void)
{
    // If a bounce is waiting
    int is_bounce_flag_on = BOUNCE_FLAG_ADDR;
    if (is_bounce_flag_on)
    {
        // Replace rpx/rpl data
        int i;

        // Get remaining size of rpx/rpl to copy and truncate it to 0x400000 bytes
        int size = MEM_SIZE;
        if (size > 0x400000)
            size = 0x400000;

        // Get where rpx/rpl code needs to be copied
        int base = 0xF6000000 + (MEM_PART * 0x400000);

        // Get current memory area
        s_mem_area* mem_area    = (s_mem_area*)(MEM_AREA);
        int mem_area_addr_start = mem_area->address;
        int mem_area_addr_end   = mem_area_addr_start + mem_area->size;
        int mem_area_offset     = MEM_OFFSET;

        // Copy rpx/rpl data
        for (i = 0; i < (size / 4); i++)
        {
            if ((mem_area_addr_start + mem_area_offset) >= mem_area_addr_end)
            {
                // Set new memory area
                mem_area            = mem_area->next;
                mem_area_addr_start = mem_area->address;
                mem_area_addr_end   = mem_area_addr_start + mem_area->size;
                mem_area_offset     = 0;
            }
            *(volatile unsigned int *)(base + i * 4) = *(volatile unsigned int *)(mem_area_addr_start + mem_area_offset);
            mem_area_offset += 4;
        }

        // Reduce size and set offset
        MEM_SIZE   = MEM_SIZE - size;
        MEM_AREA   = (int)mem_area;
        MEM_OFFSET = mem_area_offset;
        MEM_PART   = (MEM_PART + 1) % 2;

        // Bounce flag OFF
        BOUNCE_FLAG_ADDR = 0;
    }

    // return properly
    asm volatile("cmpwi 3, 0\n");
}

// A kind of magic used to store :
// - function address
// - address where to replace the instruction
// - original instruction
#define MAKE_MAGIC(x, orig_instr) { x, addr_ ## x, orig_instr}

// For every replaced functions :
struct magic_t {
    const void* func; // our replacement function which is called
    const void* call; // address where to place the jump to the our function
    uint        orig_instr;
} methods[] __attribute__((section(".magic"))) = {
    MAKE_MAGIC(LOADER_Start,                                0x5586103a),
    MAKE_MAGIC(LOADER_Entry,                                0x835e0000),
    MAKE_MAGIC(LOADER_Prep,                                 0x3f00fff9),
    MAKE_MAGIC(LiLoadRPLBasics_bef_LiWaitOneChunk,          0x809d0010),
    MAKE_MAGIC(LiLoadRPLBasics_in_1_load,                   0x7EE3BB78),
    MAKE_MAGIC(GetNextBounce_bef_LiWaitOneChunk,            0x91010008),
    MAKE_MAGIC(LiRefillBounceBufferForReading_af_getbounce, 0x2C030000),
};
