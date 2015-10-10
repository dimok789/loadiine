#include "launcher.h"

#if VER == 532
    // Includes
    #include "menu532.h"
    #include "loader532.h"
    #include "fs532.h"

    // Function definitions
    #define _Exit ((void (*)(void))0x0101cd70)
    #define OSFatal ((void (*)(char* msg))0x1031368)
    #define OSEffectiveToPhysical ((void* (*)(const void*))0x0101f510)
    #define memcpy ((void * (*)(void * dest, const void * src, int num))0x1035a6c)
    #define DCFlushRange ((void (*)(const void *addr, uint length))0x1023ee8)

    // Install addresses
    #define INSTALL_MENU_ADDR           0x011dd000  // where the menu is copied in memory
    #define INSTALL_LOADER_ADDR         0x011df000  // where the loader code is copied in memory
    #define INSTALL_FS_ADDR             0x011e0000  // where the fs functions are copied in memory

    // Install flags
    #define INSTALL_FS_DONE_ADDR        (INSTALL_FS_ADDR - 0x4) // Used to know if fs is already installed
    #define INSTALL_FS_DONE_FLAG        0xCACACACA

    // IP settings // TODO: add an option in the menu to choose if we want to use the server and then set the IP
    #define USE_SERVER                  1
//    #define SERVER_IP                   0x0A00000E
//    #define SERVER_IP                   0xC0A8BC14
    #define SERVER_IP                   0xC0A80102
//    #define SERVER_IP                   0xC0A8B203

#endif

/* Install functions */
static void InstallMenu(void);
static void InstallLoader(void);
static void InstallFS(void);

/* ****************************************************************** */
/*                               ENTRY POINT                          */
/* ****************************************************************** */
void start()
{
    /* Load a good stack */
    asm(
        "lis %r1, 0x1ab5 ;"
        "ori %r1, %r1, 0xd138 ;"
    );

    /* Make sure the kernel exploit has been run */
    if (OSEffectiveToPhysical((void *)0xa0000000) != (void *)0x10000000)
    {
        OSFatal("No ksploit.");
    }
    else
    {
        InstallMenu();
        InstallLoader();
        InstallFS();
    }
    
    _Exit();
}

/* ****************************************************************** */
/*                              INSTALL MENU                          */
/* ****************************************************************** */
static void InstallMenu(void)
{
    /* Copy menu code in memory */
    memcpy((void*)0xC1000000 + INSTALL_MENU_ADDR, menu_text_bin, menu_text_bin_len);

    /* Patch coreinit - on 5.3.2 coreinit.rpl starts at 0x101c400 */
    int jump_length = INSTALL_MENU_ADDR - 0x0101c55c;                       // => jump to (101C55C + 1C0AA4) = 11DD000 which is the codehandler
    *((volatile uint32_t *)(0xC1000000 + 0x0101c55c)) = 0x48000001 | jump_length;    // 0x481c0aa5 => bl 0x1C0AA4  => write at 0x15C in coreinit file => end of the coreinit_start function
}

/* ****************************************************************** */
/*                              INSTALL LOADER                        */
/* Copy code and replace loader instruction by call to our functions  */
/* ****************************************************************** */
static void InstallLoader(void)
{
    /* Patch for GetNextBounce function (loader) */
    /* we dont want instructions to use r9/r11 registers, as it is modified by gcc prologue/epilogue when calling our functions */
    *((volatile uint32_t *)(0xC1000000 + 0x0100b720)) = 0x3c800040; // lis r11, 0x40      (0x3d600040) => lis r4, 0x40       (0x3c800040)
    *((volatile uint32_t *)(0xC1000000 + 0x0100b718)) = 0x7ca95214; // add r9, r9, r10    (0x7d295214) => add r5, r9, r10    (0x7ca95214)
//  *((volatile uint32_t *)(0xC1000000 + 0x0100b728)) = 0x7c0a2040; // cmplw r10, r11     (0x7c0a5840) => cmplw r10, r4      (0x7c0a2040) // function call
    *((volatile uint32_t *)(0xC1000000 + 0x0100b738)) = 0x90be0090; // stw r9, 0x90(r30)  (0x913e0090) => stw r5, 0x90(r30)  (0x90be0090)

    *((volatile uint32_t *)(0xC1000000 + 0x0100b75c)) = 0x38800001; // li r11, 1          (0x39600001) => li r4, 1           (0x38800001)
    *((volatile uint32_t *)(0xC1000000 + 0x0100b764)) = 0x909e0078; // stw r11, 0x78(r30) (0x917e0078) => stw r4, 0x78(r30)  (0x909e0078)
    *((volatile uint32_t *)(0xC1000000 + 0x0100b770)) = 0x3c800040; // lis r11, 0x40      (0x3d600040) => lis r4, 0x40       (0x3c800040)
    *((volatile uint32_t *)(0xC1000000 + 0x0100b778)) = 0x7ca95214; // add r9, r9, r10    (0x7d295214) => add r5, r9, r10    (0x7ca95214)
//  *((volatile uint32_t *)(0xC1000000 + 0x0100b780)) = 0x7c0a2040; // cmplw r10, r11     (0x7c0a5840) => cmplw r10, r4      (0x7c0a2040) // function call
    *((volatile uint32_t *)(0xC1000000 + 0x0100b794)) = 0x90be0090; // stw r9, 0x90(r30)  (0x913e0090) => stw r5, 0x90(r30)  (0x90be0090)

    /* Patch to bypass SDK version tests */
    *((volatile uint32_t *)(0xC1000000 + 0x010095b4)) = 0x480000a0; // ble loc_1009654    (0x408100a0) => b loc_1009654      (0x480000a0)
    *((volatile uint32_t *)(0xC1000000 + 0x01009658)) = 0x480000e8; // bge loc_1009740    (0x408100a0) => b loc_1009740      (0x480000e8)
    
    
    /* Copy loader code in memory */
    /* - virtual address 0xA0000000 is at physical address 0x10000000 (with read/write access) */
    /* - virtual address range 0x01xxxxxx starts at physical address 0x32000000 */
    /* - we want to copy the code at INSTALL_ADDR (0x011de000), this memory range is the for cafeOS app and libraries, but is write protected */
    /* - in order to have the rights to write into memory in this address range we need to use the 0xA0000000 virtual address range */
    /* - so start virtual address is : (0xA0000000 + (0x32000000 - 0x10000000 - 0x01000000)) = 0xC1000000 */
    memcpy((void*)(INSTALL_LOADER_ADDR + 0xC1000000), loader_text_bin, loader_text_bin_len);

    /* Copy original loader instructions in memory for when we want to restore the loader at his original state */
    // TODO: copy original instructions in order to restore them later to have a clean loader state
    // we'll have to hook the "quit" function to restore the original instructions

    /* Get our functions */
    struct magic_t
    {
        const void* func; // our replacement function which is called
        const void* call; // address where to place the jump to our function
        uint        orig_instr;
    } *magic = (struct magic_t *)loader_magic_bin;
    int magic_len = sizeof(loader_magic_bin) / sizeof(struct magic_t);

    /* Replace loader instructions */
    /* Loop to replace instructions in loader code by a "bl"(jump) instruction to our replacement function */
    int i;
    for (i = 0; i < magic_len; i ++)
    {
        int func_addr  = (int)magic[i].func;
        int call_addr  = (int)magic[i].call;
//        int orig_instr = (int)magic[i].orig_instr;

        // Install function hook only if needed
        int jump_addr = func_addr - call_addr; // Compute jump length to jump from current instruction address to our function address
        *((volatile uint32_t *)(0xC1000000 + call_addr)) = 0x48000001 | jump_addr; // Replace the instruction in the loader by the jump to our function
    }
}

/* ****************************************************************** */
/*                               INSTALL FS                           */
/* Install filesystem functions                                       */
/* ****************************************************************** */
static void InstallFS(void)
{
    /* Check if already installed */
    if (*(volatile unsigned int *)(INSTALL_FS_DONE_ADDR + 0xC1000000) == INSTALL_FS_DONE_FLAG)
        return;
    *(volatile unsigned int *)(INSTALL_FS_DONE_ADDR + 0xC1000000) = INSTALL_FS_DONE_FLAG;
    
    /* Copy fs code in memory */
    unsigned int len = sizeof(fs_text_bin);
    unsigned char *loc = (unsigned char *)((char *)INSTALL_FS_ADDR + 0xC1000000);

    while (len--) {
        loc[len] = fs_text_bin[len];
    }

    /* server IP address */
#if (USE_SERVER == 1)
    *((volatile unsigned int *)loc) = SERVER_IP;
#else
    *((volatile unsigned int *)loc) = 0;
#endif

    DCFlushRange((void*)loc, sizeof(fs_text_bin));

    struct magic_t {
        void *real;
        void *replacement;
        void *call;
    } *magic = (struct magic_t *)fs_magic_bin;
    len = sizeof(fs_magic_bin) / sizeof(struct magic_t);

    volatile int *space = (volatile int *)(loc + sizeof(fs_text_bin));
    /* Patch branches to it. */
    while (len--) {
        int real_addr = (int)magic[len].real;
        int repl_addr = (int)magic[len].replacement;
        int call_addr = (int)magic[len].call;

        // set pointer to the real function
        *(volatile int *)(0xC1000000 + call_addr) = (int)space - 0xC1000000;

        // fill the pointer of the real function
        *space = *(volatile int *)(0xC1000000 + real_addr);
        space++;

        // jump to real function skipping the "mflr r0" instruction
        *space = 0x48000002 | ((real_addr + 4) & 0x03fffffc);
        space++;
        DCFlushRange((void*)space - 2, 8);

        // in the real function, replace the "mflr r0" instruction by a jump to the replacement function
        *(volatile int *)(0xC1000000 + real_addr) = 0x48000002 | (repl_addr & 0x03fffffc);
        DCFlushRange((int *)(0xC1000000 + real_addr), 4);
    }
}
