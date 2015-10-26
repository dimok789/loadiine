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
#define LI_WAIT_ONE_CHUNK3_AFTER2_NEW_INSTR     0x60000000 // nop


static inline int toupper(int c) {
    return (c >= 'a' && c <= 'z') ? (c - 0x20) : c;
}

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

    *(volatile unsigned int *)(BOUNCE_FLAG_ADDR) = 0;         // Bounce flag off
    *(volatile unsigned int *)(IS_ACTIVE_ADDR) = 0;           // replacement off
    *(volatile unsigned int *)(MEM_OFFSET) = 0;               // RPX/RPL load offset
    *(volatile unsigned int *)(MEM_AREA) = 0;                 // RPX/RPL current memory area
    *(volatile unsigned int *)(MEM_PART) = 0;                 // RPX/RPL part to fill (0=0xF6000000-0xF6400000, 1=0xF6400000-0xF6800000)
    *(volatile unsigned int *)(RPL_REPLACE_ADDR) = 0;         // Reset
    *(volatile unsigned int *)(RPL_ENTRY_INDEX_ADDR) = 0;     // Reset
    *(volatile unsigned int *)(IS_LOADING_RPX_ADDR) = 1;      // Set RPX loading

    // Disable fs
    if (*(volatile unsigned int *)0xEFE00000 != RPX_CHECK_NAME)
    {
        *(volatile unsigned int *)(RPX_NAME) = 0;

        #if (IS_USING_MII_MAKER == 1)
            *(volatile unsigned int*)(GAME_LAUNCHED) = 0;
        #endif
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
    *(volatile unsigned int *)(RPL_REPLACE_ADDR) = 0;      // Reset
    *(volatile unsigned int *)(IS_LOADING_RPX_ADDR) = 0;   // Set RPL loading

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
    *(volatile unsigned int *)(IS_ACTIVE_ADDR) = 0; // Set as inactive

    // If we are in Smash Bros app or Mii Maker
#if (IS_USING_MII_MAKER == 0)
    if (*(volatile unsigned int *)0xEFE00000 == RPX_CHECK_NAME)
#else
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (*(volatile unsigned int*)(GAME_LAUNCHED) == 1))
#endif
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
                *(volatile unsigned int *)(RPL_REPLACE_ADDR) = 1;
                *(volatile unsigned int *)(RPL_ENTRY_INDEX_ADDR) = (unsigned int)rpl_struct;

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
#if (IS_USING_MII_MAKER == 0)
    if (*(volatile unsigned int *)0xEFE00000 == RPX_CHECK_NAME)
#else
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (*(volatile unsigned int*)(GAME_LAUNCHED) == 1))
#endif
    {
        s_rpx_rpl *entry = 0;

        if (*(volatile unsigned int *)(RPL_REPLACE_ADDR) == 1)
        {
            entry = (s_rpx_rpl *)( *(volatile unsigned int *)(RPL_ENTRY_INDEX_ADDR) );
        }
        else if (*(volatile unsigned int *)(IS_LOADING_RPX_ADDR) == 1)
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
            *(volatile unsigned int*)(r3) = size;
            *(volatile unsigned int*)(0xEFE0560C) = size;
            *(volatile unsigned int*)(0xEFE05658) = size;
            *(volatile unsigned int*)(0xEFE05660) = entry->size;
            *(volatile unsigned int*)(0xEFE0566C) = *(volatile unsigned int *)(MEM_SIZE);
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
    int r23;
    asm volatile("mr %0, 23\n"
        :"=r"(r23)
        :
        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    );

    // Logs
//    loader_debug_t * loader = (loader_debug_t *)DATA_ADDR;
//    while(loader->tag != 0)
//        loader++;
//    loader[0].tag = 4;
//    loader[0].data = *(volatile unsigned int *)0xEFE00000;
//    loader[1].tag = 0;

    // If we are in Smash Bros app
#if (IS_USING_MII_MAKER == 0)
    if (*(volatile unsigned int *)0xEFE00000 == RPX_CHECK_NAME)
#else
    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (*(volatile unsigned int*)(GAME_LAUNCHED) == 1))
#endif
    {
        // Check if it is an rpx, look for rpx name, if it is rpl, it is already marked
        int is_rpx = 0;
        s_rpx_rpl * rpx_rpl_entry = 0;

        // Check if rpl is already marked
        if (*(volatile unsigned int *)(RPL_REPLACE_ADDR) == 1)
        {
            rpx_rpl_entry = (s_rpx_rpl *) (*(volatile unsigned int *)(RPL_ENTRY_INDEX_ADDR));

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
                *(volatile unsigned int*)(RPL_REPLACE_ADDR) = 0;
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
            *(volatile unsigned int *)(MEM_SIZE)    = rpx_rpl_entry->size - size;
            *(volatile unsigned int *)(MEM_AREA)    = (int)mem_area;
            *(volatile unsigned int *)(MEM_OFFSET)  = mem_area_offset;
            *(volatile unsigned int *)(MEM_PART)    = 1;

            // If rpx/rpl size is less than 0x400000 bytes, we need to stop here
            if (is_rpx)
            {
                // these memories are still different in their values, tried to overwrite them but i think its the wrong place yet
                //*(volatile unsigned int *)(0xEFE01010) = 0x03FFCFC0;
                //*(volatile unsigned int *)(0xEFE01014) = 0x04000000;
                //*(volatile unsigned int *)(0xEFE054F4) = 0x0003CD9D;
                //*(volatile unsigned int *)(0xEFE05534) = 0x01014EF3;
                //*(volatile unsigned int *)(0xEFE05548) = 0x0000093D;
                //*(volatile unsigned int *)(0xEFE06580) = 0x00000031;
                //*(volatile unsigned int *)(0xEFE065A8) = 0x31000000;
                //*(volatile unsigned int *)(0xEFE06664) = 0x20310A00;
                //*(volatile unsigned int *)(0xEFE06774) = 0x00000001;
                //*(volatile unsigned int *)(0xEFE096F8) = 0x00000014;
                //*(volatile unsigned int *)(0xEFE096FC) = 0x000618E5;
                //*(volatile unsigned int *)(0xEFE0A414) = 0x04000000;
                //*(volatile unsigned int *)(0xEFE0A418) = 0x3BFA0000;
                //*(volatile unsigned int *)(0xEFE0A41C) = 0x00000001;
                //*(volatile unsigned int *)(0xEFE06774) = 0xEFE0BA00;
                //*(volatile unsigned int *)(0xEFE1AD28) = 0x00000001;

                // Remaining size to load is in one (or more) of those addresses
                if (*(volatile unsigned int *)(MEM_SIZE) == 0)
                {
                    *(volatile unsigned int *)(0xEFE05790) = size;
                    *(volatile unsigned int *)(0xEFE054EC) = size;
                    *(volatile unsigned int *)(0xEFE05580) = size;
                    *(volatile unsigned int *)(0xEFE19D0C) = size;
                    *(volatile unsigned int *)(0xEFE1CF84) = size;
                    *(volatile unsigned int *)(0xEFE1D820) = size;
                }
                else {
                    // set address to maximum size to signalize there is more data to come
                    *(volatile unsigned int *)(0xEFE05790) = 0x400000;
                    *(volatile unsigned int *)(0xEFE054EC) = 0x400000;
                    *(volatile unsigned int *)(0xEFE05580) = 0x400000;
                    *(volatile unsigned int *)(0xEFE19D0C) = 0x400000;
                    *(volatile unsigned int *)(0xEFE1CF84) = 0x400000;
                    *(volatile unsigned int *)(0xEFE1D820) = 0x400000;
                }

                // Set rpx name as active for FS to replace file from sd card
                *(volatile unsigned int *)(RPX_NAME) = *(volatile unsigned int *)(RPX_NAME_PENDING);
            }
            else
            {
                // TODO: check those for mii maker, probably needs change
                // set remaining size
                *(volatile unsigned int *)(0xEFE0647C) = size;
                *(volatile unsigned int *)(0xEFE06510) = size;
                *(volatile unsigned int *)(0xEFE06728) = size;
                *(volatile unsigned int *)(0xEFE19D0C) = size;
                *(volatile unsigned int *)(0xEFE1D004) = size;
                *(volatile unsigned int *)(0xEFE1D820) = size;
            }

            // Replacement ON
            *(volatile unsigned int *)(IS_ACTIVE_ADDR) = 1;
        }
    }

    // return properly
    asm volatile("mr 3, %0\n"
        :
        :"r"(r23)
    );
}

/* LiSetupOneRPL_after *********************************************************
 * - instruction address = 0x0100CDC0
 * - original instruction = 0x7FC3F378 : "mr r3, r30"
 */
//void LiSetupOneRPL_after(void)
//{
//    // Save registers
//    int r30;
//    asm volatile("mr %0, 30\n"
//        :"=r"(r30)
//        :
//        :"memory", "ctr", "lr", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
//    );
//
//    // Do our stuff
//#if (IS_USING_MII_MAKER == 0)
//    if (*(volatile unsigned int *)0xEFE00000 == RPX_CHECK_NAME)
//#else
//    if (*(volatile unsigned int*)0xEFE00000 == RPX_CHECK_NAME && (*(volatile unsigned int*)(GAME_LAUNCHED) == 1))
//#endif
//    {
//        //*(volatile unsigned int *)(IS_ACTIVE_ADDR) = 0; // Set as inactive
//
//        // Restore original instruction after LiWaitOneChunk (in GetNextBounce) // TODO: check if it is needed
////        *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK3_AFTER1_ADDR)) = LI_WAIT_ONE_CHUNK3_AFTER1_ORIG_INSTR;
////        *((volatile unsigned int*)(0xC1000000 + LI_WAIT_ONE_CHUNK3_AFTER2_ADDR)) = LI_WAIT_ONE_CHUNK3_AFTER2_ORIG_INSTR;
////
////        FlushBlock(LI_WAIT_ONE_CHUNK3_AFTER1_ADDR);
////        FlushBlock(LI_WAIT_ONE_CHUNK3_AFTER2_ADDR);
//    }
//
//    // Return properly
//    asm volatile("mr 3, %0\n"
//        :
//        :"r"(r30)
//        );
//    return;
//}

//
//int GetNextBounce(unsigned char *buffer)
//{
//    int r4 = *(unsigned int *)(&buffer[0x14]);
//    int r5  = *(unsigned int *)(&buffer[0x8C]);
//    int size = 0;
//
//    int result = LiWaitOneChunk(&size, r4, r5);
//
//    // If it is active
//    int is_active = *(volatile unsigned int *)(IS_ACTIVE_ADDR);
//    if (is_active)
//    {
//        // Check if we need to adjust the read size and offset
//        int s = *(volatile unsigned int *)(MEM_SIZE);
//        if (s >= 0x400000) {
//            s = 0x400000;
//            result = 0;
//        }
//        size = s;         // set the new read size
//        //r5 = r12 + r10;     // set the offset end
//
//        *(volatile unsigned int *)(BOUNCE_FLAG_ADDR) = 1; // Bounce flag on
//    }
//
//    if(result)
//    {
//        return result;
//    }
//
//    if(*(unsigned int *)(&buffer[0x78]) == 1)
//    {
//        *(unsigned int *)(&buffer[0x78]) = 2;
//        *(unsigned int *)(&buffer[0x80]) += size;
//        *(unsigned int *)(&buffer[0x7c]) = *(unsigned int *)(&buffer[0x94]);
//        *(unsigned int *)(&buffer[0x84]) = *(unsigned int *)(&buffer[0x88]);
//        *(unsigned int *)(&buffer[0x90]) += size;
//        *(unsigned int *)(&buffer[0x88]) += size;
//
//        if(size != 0x400000)
//        {
//           LiInitBuffer();
//        }
//        else
//        {
//            result = LiRefillUpcomingBounceBuffer(buffer, 1);
//        }
//    }
//    else
//    {
//        *(unsigned int *)(&buffer[0x78]) = 1;
//        *(unsigned int *)(&buffer[0x84]) = *(unsigned int *)(&buffer[0x88]);
//        *(unsigned int *)(&buffer[0x88]) += size;
//        *(unsigned int *)(&buffer[0x7c]) =  *(unsigned int *)(&buffer[0x94]);
//        *(unsigned int *)(&buffer[0xB0]) -= *(unsigned int *)(&buffer[0x80]);
//        *(unsigned int *)(&buffer[0x80]) = size;
//        *(unsigned int *)(&buffer[0x90]) += size;
//
//        if(size != 0x400000)
//        {
//           LiInitBuffer();
//        }
//        else {
//            result = LiRefillUpcomingBounceBuffer(buffer, 2);
//        }
//    }
//
//    return result;
//}
//
/* GetNextBounce_1 **************************************************************
 * - instruction address = 0x0100b728
 * - original instruction = 0x7c0a2040 : "cmplw r10, r4" (instruction is originaly "cmplw r10, r11" but it was patched because of the gcc proploge overwriting r11)
 */
void GetNextBounce_1(void)
{
    // Save registers
    int r10; // size read, max 0x400000, or less if it is end of stream
    int r12; // read offset start : starts at 0x400000 + x * 0x800000
    int r5;  // read offset end   : read offset start + size read
    int r6, r9, r30;
    asm volatile("mr %0, 10\n"
        "mr %1, 12\n"
        "mr %2, 5\n" // r5 is originaly r9 but we patched it
        "mr %3, 30\n"
        "mr %4, 6\n"         // read out r6 which is than transformed to r9 by substract of r10
        :"=r"(r10), "=r"(r12), "=r"(r5), "=r"(r30), "=r"(r9)
        :
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "12"
    );

    // Logs
//    loader_debug_t * loader = (loader_debug_t *)DATA_ADDR;
//    while(loader->tag != 0)
//        loader++;
//    loader[0].tag = 5;
//    loader[0].data = *(volatile unsigned int *)0xEFE00000;
//    loader[1].tag = 0;

    // substract r10 to get the value of r9 as what we loaded is actually r6
    r9 = r9 - r10;

    // If it is active
    int is_active = *(volatile unsigned int *)(IS_ACTIVE_ADDR);
    if (is_active)
    {
        // Check if we need to adjust the read size and offset
        int size = *(volatile unsigned int *)(MEM_SIZE);
        if (size > 0x400000)
            size = 0x400000;

        r10 = size;         // set the new read size
        r5 = r12 + r10;     // set the offset end

        *(volatile unsigned int *)(BOUNCE_FLAG_ADDR) = 1; // Bounce flag on
    }

    // store correct size
    r6 = r9 + r10;

    // return properly
    asm volatile("mr 12, %0\n"
        "mr 5, %1\n"
        "mr 10, %2\n"
        "mr 6, %3\n"
        "mr 30, %4\n"
        "stw 6, 0x80(30)\n"
        "cmplw 10, 4\n"
        :
        :"r"(r12), "r"(r5), "r"(r10), "r"(r6), "r"(r30)
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "12"
    );
}

/* GetNextBounce_2 **************************************************************
 * - instruction address = 0x0100b780
 * - original instruction = 0x7c0a2040 : "cmplw r10, r4" (instruction is originaly "cmplw r10, r11" but it's been patched because of the gcc proploge overwriting r11)
 */
void GetNextBounce_2(void)
{
    // Save registers
    int r10; // size read, max 0x400000, or less if it is end of stream
    int r12; // read offset start : starts at 0x800000 + x * 0x800000
    int r5;  // read offset end   : read offset start + size read
    int r0, r30;
    asm volatile("mr %0, 10\n"
        "mr %1, 12\n"
        "mr %2, 30\n"
        "mr %3, 5\n" // r5 is originaly r9 but it's been patched
        :"=r"(r10), "=r"(r12), "=r"(r30), "=r"(r5)
        :
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "12"
    );

    // Logs
//    loader_debug_t * loader = (loader_debug_t *)DATA_ADDR;
//    while(loader->tag != 0)
//        loader++;
//    loader[0].tag = 6;
//    loader[0].data = *(volatile unsigned int *)0xEFE00000;
//    loader[1].tag = 0;

    // If we are in smash bros app
    int is_active = *(volatile unsigned int *)(IS_ACTIVE_ADDR);
    if (is_active)
    {
        // Check if we need to adjust the read size and offset
        int size = *(volatile unsigned int *)(MEM_SIZE);
        if (size > 0x400000)
            size = 0x400000;

        r10 = size;         // set the new read size
        r5 = r12 + r10;     // set the offset end

        *(volatile unsigned int *)(BOUNCE_FLAG_ADDR) = 1; // Bounce flag on
    }

    r0 = r12 + r10;

    // return properly
    asm volatile("mr 12, %0\n"
        "mr 5, %1\n"
        "mr 10, %2\n"
        "mr 0, %3\n"
        "mr 30, %4\n"
        "stw 0, 0x88(30)\n"
        "cmplw 10, 4\n"
        :
        :"r"(r12), "r"(r5), "r"(r10), "r"(r0), "r"(r30)
        :"memory", "ctr", "lr", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "12"
    );
}

/* LiRefillBounceBufferForReading_af_getbounce *********************************
 * - instruction address = 0x0100BA28
 * - original instruction = 0x2C030000 : "cmpwi r3, 0"
 */
void LiRefillBounceBufferForReading_af_getbounce(void)
{
    // Logs
//    loader_debug_t * loader = (loader_debug_t *)DATA_ADDR;
//    while(loader->tag != 0)
//        loader++;
//    loader[0].tag = 5;
//    loader[0].data = *(volatile unsigned int *)0xEFE00000;
//    loader[1].tag = 0;

    // If a bounce is waiting
    int is_bounce_flag_on = *(volatile unsigned int *)(BOUNCE_FLAG_ADDR);
    if (is_bounce_flag_on)
    {
        // Replace rpx/rpl data
        int i;

        // Get remaining size of rpx/rpl to copy and truncate it to 0x400000 bytes
        int size = *(volatile unsigned int *)(MEM_SIZE);
        if (size > 0x400000)
            size = 0x400000;

        // Get where rpx/rpl code needs to be copied
        int base = 0xF6000000 + (*(volatile unsigned int *)(MEM_PART) * 0x400000);

        // Get current memory area
        s_mem_area* mem_area    = (s_mem_area*)(*(volatile unsigned int *)(MEM_AREA));
        int mem_area_addr_start = mem_area->address;
        int mem_area_addr_end   = mem_area_addr_start + mem_area->size;
        int mem_area_offset     = *(volatile unsigned int *)(MEM_OFFSET);

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
        *(volatile unsigned int*)(MEM_SIZE)   = *(volatile unsigned int*)(MEM_SIZE) - size;
        *(volatile unsigned int*)(MEM_AREA)   = (int)mem_area;
        *(volatile unsigned int*)(MEM_OFFSET) = mem_area_offset;
        *(volatile unsigned int*)(MEM_PART)   = (*(volatile unsigned int *)(MEM_PART) + 1) % 2;

        // Replace remaining size in loader memory
        //if (size != 0x400000) // TODO: Check if this test is needed
        {
            //*(volatile unsigned int *)(0xEFE0552C) = size;  // this is actually 0x001A001A, don't think its correct to overwrite this
            //*(volatile unsigned int *)(0xEFE05588) = size;  // this is a pointer to 0xEFE055EC, so better not overwrite it
            //*(volatile unsigned int *)(0xEFE1D820) = size;    // this one has 0x000C3740 on smash bros, doesn't look like remaining size?

            //*(volatile unsigned int*)(0xEFE0560C) = size;
            //*(volatile unsigned int*)(0xEFE05658) = size;
            *(volatile unsigned int*)(0xEFE1D004) = size;
            *(volatile unsigned int*)(0xEFE1D820) = size;
        }

        // Bounce flag OFF
        *(volatile unsigned int *)(BOUNCE_FLAG_ADDR) = 0;
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
//    MAKE_MAGIC(LiSetupOneRPL_after,                         0x7FC3F378),   // this function does nothing anymore, don't need to replace
//    MAKE_MAGIC(GetNextBounce,                               0x7C0802A6),
//    MAKE_MAGIC(sLiRefillBounceBufferForReading,             0x7C0802A6),
    MAKE_MAGIC(GetNextBounce_1,                             0x7c0a2040),
    MAKE_MAGIC(GetNextBounce_2,                             0x7c0a2040),
    MAKE_MAGIC(LiRefillBounceBufferForReading_af_getbounce, 0x2C030000),
};
