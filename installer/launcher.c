#include "launcher.h"
#include "elf_abi.h"
#include "../src/common/common.h"
#include "../../libwiiu/src/coreinit.h"
#include "../../libwiiu/src/vpad.h"
#include "../../libwiiu/src/socket.h"

#if VER == 532
    #define CODE_RW_BASE_OFFSET     0xBC000000
    #define DATA_RW_BASE_OFFSET     0

    // Function definitions
    #define SYSLaunchMiiStudio ((void (*)(void))0xDEAAEB8)
    #define _Exit ((void (*)(void))0x0101cd70)
    #define OSEffectiveToPhysical ((void* (*)(const void*))0x0101f510)
    #define memcpy ((void * (*)(void * dest, const void * src, int num))0x1035a6c)
    #define DCFlushRange ((void (*)(const void *addr, uint length))0x1023ee8)
    #define ICInvalidateRange ((void (*)(const void *addr, uint length))0x1024010)
#endif

#define PRINT_TEXT1(x, y, str) { OSScreenPutFontEx(1, x, y, str); }
#define PRINT_TEXT2(x, y, _fmt, ...) { __os_snprintf(msg, 80, _fmt, __VA_ARGS__); OSScreenPutFontEx(1, x, y, msg); }
#define BTN_PRESSED (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN)


typedef union u_serv_ip
{
    uint8_t  digit[4];
    uint32_t full;
} u_serv_ip;

typedef struct {
    char tag[8];                     /* 0x000 "OSContxt" */
    int32_t gpr[32];                 /* 0x008 from OSDumpContext */
    uint32_t cr;                     /* 0x088 from OSDumpContext */
    uint32_t lr;                     /* 0x08c from OSDumpContext */
    uint32_t ctr;                    /* 0x090 from context switch code */
    uint32_t xer;                    /* 0x094 from context switch code */
    uint32_t srr0;                   /* 0x098 from OSDumpContext */
    uint32_t srr1;                   /* 0x09c from OSDumpContext */
    char _unknowna0[0xb8 - 0xa0];
    uint64_t fpr[32];                /* 0x0b8 from OSDumpContext */
    int16_t spinLockCount;           /* 0x1b8 from OSDumpContext */
    char _unknown1ba[0x1bc - 0x1ba]; /* 0x1ba could genuinely be padding? */
    uint32_t gqr[8];                 /* 0x1bc from OSDumpContext */
    char _unknown1dc[0x1e0 - 0x1dc];
    uint64_t psf[32];                /* 0x1e0 from OSDumpContext */
    int64_t coretime[3];             /* 0x2e0 from OSDumpContext */
    int64_t starttime;               /* 0x2f8 from OSDumpContext */
    int32_t error;                   /* 0x300 from OSDumpContext */
    char _unknown304[0x6a0 - 0x304];
} __attribute__((packed)) OSThread;  /* 0x6a0 total length from RAM dumps */

typedef struct {
    unsigned char *data;
    int len;
    int alloc_size;

    void*(*MEMAllocFromDefaultHeapEx)(unsigned int size, unsigned int align);
    void(*MEMFreeToDefaultHeap)(void *ptr);
} file_struct_t;

typedef struct {
    unsigned char *data_elf;
    unsigned int coreinit_handle;
    /* function pointers */
    void*(*memset)(void * dest, unsigned int value, unsigned int bytes);

    void*(*MEMAllocFromDefaultHeapEx)(unsigned int size, unsigned int align);
    void(*MEMFreeToDefaultHeap)(void *ptr);

    void*(*curl_easy_init)();
    void(*curl_easy_setopt)(void *handle, unsigned int param, void *op);
    int(*curl_easy_perform)(void *handle);
    void(*curl_easy_getinfo)(void *handle, unsigned int param, void *op);
    void (*curl_easy_cleanup)(void *handle);
} private_data_t;

/* Install functions */
static void InstallMain(private_data_t *private_data);
static void InstallMenu(private_data_t *private_data);
static void InstallLoader(private_data_t *private_data);
static void InstallFS(private_data_t *private_data);

static int show_ip_selection_screen(unsigned int coreinit_handle, unsigned int *ip_address);
static void curl_thread_callback(int argc, void *argv);

static void SetupKernelSyscall(unsigned int addr);

/* assembly functions */
extern void Syscall_0x36(void);
extern void KernelPatches(void);

/* ****************************************************************** */
/*                               ENTRY POINT                          */
/* ****************************************************************** */
void _start()
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
        private_data_t private_data;

        /* Get coreinit handle and keep it in memory */
        unsigned int coreinit_handle;
        OSDynLoad_Acquire("coreinit", &coreinit_handle);
        private_data.coreinit_handle = coreinit_handle;

        /* Get our memory functions */
        void*(*memset)(void * dest, unsigned int value, unsigned int bytes);
        OSDynLoad_FindExport(coreinit_handle, 0, "memset", &memset);
        memset(&private_data, 0, sizeof(private_data_t));
        private_data.memset = memset;

        unsigned int *functionPointer = 0;
        OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeapEx", &functionPointer);
        private_data.MEMAllocFromDefaultHeapEx = (void*(*)(unsigned int size, unsigned int align))*functionPointer;
        OSDynLoad_FindExport(coreinit_handle, 1, "MEMFreeToDefaultHeap", &functionPointer);
        private_data.MEMFreeToDefaultHeap = (void (*)(void *))*functionPointer;

        /* Get functions to send restart signal */
        int(*IM_Open)();
        int(*IM_Close)(int fd);
        int(*IM_SetDeviceState)(int fd, void *mem, int state, int a, int b);
        void*(*OSAllocFromSystem)(unsigned int size, int align);
        void(*OSFreeToSystem)(void *ptr);
        OSDynLoad_FindExport(coreinit_handle, 0, "IM_Open", &IM_Open);
        OSDynLoad_FindExport(coreinit_handle, 0, "IM_Close", &IM_Close);
        OSDynLoad_FindExport(coreinit_handle, 0, "IM_SetDeviceState", &IM_SetDeviceState);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSAllocFromSystem", &OSAllocFromSystem);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSFreeToSystem", &OSFreeToSystem);

        /* Send restart signal to get rid of uneeded threads */
        /* Cause the other browser threads to exit */
        int fd = IM_Open();
        void *mem = OSAllocFromSystem(0x100, 64);
        memset(mem, 0, 0x100);

        /* Sets wanted flag */
        IM_SetDeviceState(fd, mem, 3, 0, 0);
        IM_Close(fd);
        OSFreeToSystem(mem);

        /* Waits for thread exits */
        unsigned int t1 = 0x1FFFFFFF;
        while(t1--) ;

        /* Prepare for thread startups */
        int (*OSCreateThread)(OSThread *thread, void *entry, int argc, void *args, unsigned int stack, unsigned int stack_size, int priority, unsigned short attr);
        int (*OSResumeThread)(OSThread *thread);
        int (*OSIsThreadTerminated)(OSThread *thread);

        OSDynLoad_FindExport(coreinit_handle, 0, "OSCreateThread", &OSCreateThread);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSResumeThread", &OSResumeThread);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSIsThreadTerminated", &OSIsThreadTerminated);

        /* Allocate a stack for the thread */
        /* IMPORTANT: libcurl uses around 0x1000 internally so make
            sure to allocate more for the stack to avoid crashes */
        void *stack = private_data.MEMAllocFromDefaultHeapEx(0x2000, 0x20);
        /* Create the thread variable */
        OSThread *thread = (OSThread *) private_data.MEMAllocFromDefaultHeapEx(sizeof(OSThread), 8);
        if(!thread || !stack)
            OSFatal("Thread memory allocation failed.");

        // the thread stack is too small on current thread, switch to an own created thread
        // create a detached thread with priority 0 and use core 1
        int ret = OSCreateThread(thread, curl_thread_callback, 1, (void*)&private_data, (unsigned int)stack+0x2000, 0x2000, 0, 0x1A);
        if (ret == 0)
            OSFatal("Failed to create thread");

        /* Schedule it for execution */
        OSResumeThread(thread);

        /* while we are downloading let the user select his IP stuff */
        unsigned int ip_address = 0;
        int result = show_ip_selection_screen(coreinit_handle, &ip_address);

        // Keep this main thread around for ELF loading
        // Can not use OSJoinThread, which hangs for some reason, so we use a detached one and wait for it to terminate
        while(OSIsThreadTerminated(thread) == 0)
        {
            asm volatile (
            "    nop\n"
            "    nop\n"
            "    nop\n"
            "    nop\n"
            "    nop\n"
            "    nop\n"
            "    nop\n"
            "    nop\n"
            );
        }

        /* Free thread memory and stack */
        private_data.MEMFreeToDefaultHeap(thread);
        private_data.MEMFreeToDefaultHeap(stack);

        /* Install our ELF files */
        if(result){
            /* patch server IP */
            SERVER_IP = ip_address;
            /* Set GAME_LAUNCHED to 0 */
            GAME_LAUNCHED = 0;
            GAME_RPX_LOADED = 0;
            RPX_CHECK_NAME = 0xDEADBEAF;
            /* Set LOADIINE mode to smash bros initially */
            LOADIINE_MODE = LOADIINE_MODE_SMASH_BROS;
            /* Install our code now */
            InstallMain(&private_data);

            /* setup our own syscall and call it */
            SetupKernelSyscall((unsigned int)KernelPatches);
            Syscall_0x36();

            InstallMenu(&private_data);
            InstallLoader(&private_data);
            InstallFS(&private_data);
        }

        /* free memory allocated */
        if(private_data.data_elf) {
            private_data.MEMFreeToDefaultHeap(private_data.data_elf);
        }
    }

    _Exit();
}

/* *****************************************************************************
 * Base functions
 * ****************************************************************************/

/* Write a 32-bit word with kernel permissions */
static void kern_write(uint32_t addr, uint32_t value)
{
	asm volatile(
		"li 3,1\n"
		"li 4,0\n"
		"mr 5,%1\n"
		"li 6,0\n"
		"li 7,0\n"
		"lis 8,1\n"
		"mr 9,%0\n"
		"mr %1,1\n"
		"li 0,0x3500\n"
		"sc\n"
		"nop\n"
		"mr 1,%1\n"
		:
		:	"r"(addr), "r"(value)
		:	"memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
			"11", "12"
		);
}

#define KERN_SYSCALL_TBL_5          0xFFEAA0E0 // works with browser

static void SetupKernelSyscall(unsigned int address)
{
    // Add syscall #0x36
    kern_write(KERN_SYSCALL_TBL_5 + (0x36 * 4), address);
}

/* IP selection screen implemented by Maschell */
static int show_ip_selection_screen(unsigned int coreinit_handle, unsigned int *ip_address)
{
    /************************************************************************/
    // Prepare screen
    void (*OSScreenInit)();
    unsigned int (*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
    unsigned int (*OSScreenSetBufferEx)(unsigned int bufferNum, void * addr);
    unsigned int (*OSScreenClearBufferEx)(unsigned int bufferNum, unsigned int temp);
    unsigned int (*OSScreenFlipBuffersEx)(unsigned int bufferNum);
    unsigned int (*OSScreenPutFontEx)(unsigned int bufferNum, unsigned int posX, unsigned int posY, void * buffer);

    OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenInit", &OSScreenInit);
    OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
    OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenSetBufferEx", &OSScreenSetBufferEx);
    OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenClearBufferEx", &OSScreenClearBufferEx);
    OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenFlipBuffersEx", &OSScreenFlipBuffersEx);
    OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenPutFontEx", &OSScreenPutFontEx);


    /* ****************************************************************** */
    /*                             IP Settings                            */
    /* ****************************************************************** */

    // Set server ip
    u_serv_ip ip;
    ip.full = PC_IP;
    char msg[80];
    // Prepare screen
    int screen_buf0_size = 0;
    int screen_buf1_size = 0;
    uint screen_color = 0; // (r << 24) | (g << 16) | (b << 8) | a;

    // Init screen and screen buffers
    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(0);
    screen_buf1_size = OSScreenGetBufferSizeEx(1);
    OSScreenSetBufferEx(0, (void *)0xF4000000);
    OSScreenSetBufferEx(1, (void *)0xF4000000 + screen_buf0_size);

    // Clear screens
    OSScreenClearBufferEx(0, screen_color);
    OSScreenClearBufferEx(1, screen_color);

    // Flush the cache
    DCFlushRange((void *)0xF4000000, screen_buf0_size);
    DCFlushRange((void *)0xF4000000 + screen_buf0_size, screen_buf1_size);

    // Flip buffers
    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);

    // Prepare vpad
    unsigned int vpad_handle;
    int (*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
    OSDynLoad_Acquire("vpad.rpl", &vpad_handle);
    OSDynLoad_FindExport(vpad_handle, 0, "VPADRead", &VPADRead);

    // Set server ip with buttons
    uint8_t sel_ip = 3;
    int error;
    uint8_t update_screen = 1;         // refresh screen initially
    VPADData vpad_data;
    VPADRead(0, &vpad_data, 1, &error); //Read initial vpad status
    int noip = 1;
    int result = 0;
    int delay = 0;

    while (1)
    {
        // Print message
        PRINT_TEXT1(24, 1, "-- LOADIINE --");
        PRINT_TEXT1(0, 5, "1. Press A to install loadiine");

        PRINT_TEXT1(0, 11, "2    (optional)");
        PRINT_TEXT2(0, 12, "%s : %3d.%3d.%3d.%3d", "  a. Server IP", ip.digit[0], ip.digit[1], ip.digit[2], ip.digit[3]);
        PRINT_TEXT1(0, 13, "  b. Press X to install loadiine with server settings");
        PRINT_TEXT1(42, 17, "home button to exit ...");

        // Print ip digit selector
        uint8_t x_shift = 17 + 4 * sel_ip;
        PRINT_TEXT1(x_shift, 11, "vvv");

        // Refresh screen if needed
        if (update_screen)
        {
            OSScreenFlipBuffersEx(1);
            OSScreenClearBufferEx(1, 0);
        }

        // Read vpad
        VPADRead(0, &vpad_data, 1, &error);

        // Check for buttons
        // Home Button
        if (vpad_data.btn_trigger & BUTTON_HOME) {
            result = 0;
            break;
        }
        // A Button
        if (vpad_data.btn_trigger & BUTTON_A) {
            *ip_address = 0;
            result = 1;
            break;
        }
        // A Button
        if (vpad_data.btn_trigger & BUTTON_X){
            *ip_address = ip.full;
            result = 1;
            break;
        }
        // Left/Right Buttons
        if (vpad_data.btn_trigger & BUTTON_LEFT ) sel_ip = !sel_ip ? sel_ip = 3 : --sel_ip;
        if (vpad_data.btn_trigger & BUTTON_RIGHT) sel_ip = ++sel_ip % 4;

        // Up/Down Buttons
        if ((vpad_data.btn_hold & BUTTON_UP) || (vpad_data.btn_trigger & BUTTON_UP)) {
            if(--delay <= 0) {
                ip.digit[sel_ip]++;
                delay = (vpad_data.btn_trigger & BUTTON_UP) ? 30 : 3;
            }
        }
        else if ((vpad_data.btn_hold & BUTTON_DOWN) || (vpad_data.btn_trigger & BUTTON_DOWN)) {
            if(--delay <= 0) {
                ip.digit[sel_ip]--;
                delay = (vpad_data.btn_trigger & BUTTON_DOWN) ? 30 : 3;
            }
        }
        else {
            delay = 0;
        }

        // Button pressed ?
        update_screen = (vpad_data.btn_hold & BTN_PRESSED) ? 1 : 0;
    }

    return result;
}

/* libcurl data write callback */
static int curl_write_data_callback(void *buffer, int size, int nmemb, void *userp)
{
    file_struct_t *file = (file_struct_t *)userp;
    int insize = size*nmemb;

    while(file->len + insize > file->alloc_size)  {
        if(file->data) {
            file->MEMFreeToDefaultHeap(file->data);
        }
        file->alloc_size += 0x20000;
        file->data = file->MEMAllocFromDefaultHeapEx(file->alloc_size, 0x20);
        if(!file->data)
            OSFatal("Memory allocation failed");
    }

    memcpy(file->data + file->len, buffer, insize);
    file->len += insize;
    return insize;
}

/* The downloader thread */
#define CURLOPT_WRITEDATA 10001
#define CURLOPT_URL 10002
#define CURLOPT_WRITEFUNCTION 20011
#define CURLINFO_RESPONSE_CODE 0x200002

static int curl_download_file(private_data_t *private_data, void * curl, char *url, unsigned char **out_buffer)
{
    file_struct_t file;
    file.MEMAllocFromDefaultHeapEx = private_data->MEMAllocFromDefaultHeapEx;
    file.MEMFreeToDefaultHeap = private_data->MEMFreeToDefaultHeap;
    file.data = 0;
    file.len = 0;
    file.alloc_size = 0;
    private_data->curl_easy_setopt(curl, CURLOPT_URL, url);
    private_data->curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_callback);
    private_data->curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

    /* Download file */
    int ret = private_data->curl_easy_perform(curl);
    if(ret)
        OSFatal(url);

    /* Do error checks */
    if(!file.len) {
        OSFatal(url);
    }

    int resp = 404;
    private_data->curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp);
    if(resp != 200) {
        OSFatal(url);
    }

    *out_buffer = file.data;
    return file.len;
}

static void curl_thread_callback(int argc, void *argv)
{
    private_data_t *private_data = (private_data_t *)argv;
    char *leaddr = (char*)0;
    unsigned char *str;

    char buf[128];
    /* find address left in ram */
    for(str = (unsigned char*)0x1A000000; str < (unsigned char*)0x20000000; str++)
    { /* Search for /payload which indicates the current address */
        if(*(unsigned int*)str == 0x2F706179 && *(unsigned int*)(str+4) == 0x6C6F6164)
        {
            leaddr = (char*)str;
            while(*leaddr)
                leaddr--;
            leaddr++;
            /* If string starts with http its likely to be correct */
            if(*(unsigned int*)leaddr == 0x68747470)
                break;
            leaddr = (char*)0;
        }
    }
    if(leaddr == (char*)0)
        OSFatal("URL not found");

    /* Acquire and setup libcurl */
    unsigned int libcurl_handle;
    OSDynLoad_Acquire("nlibcurl", &libcurl_handle);

    OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_init", &private_data->curl_easy_init);
    OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_setopt", &private_data->curl_easy_setopt);
    OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_perform", &private_data->curl_easy_perform);
    OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_getinfo", &private_data->curl_easy_getinfo);
    OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_cleanup", &private_data->curl_easy_cleanup);

    void *curl = private_data->curl_easy_init();
    if(!curl) {
        OSFatal("cURL init failed");
    }

    /* Generate the url address */
    char *src_ptr = leaddr;
    char *ptr = buf;
    while(*src_ptr) {
        *ptr++ = *src_ptr++;
    }
    *ptr = 0;
    // go back to last /
    while(*ptr != 0x2F)
        ptr--;

    memcpy(ptr+1, "loadiine.elf", 13);
    curl_download_file(private_data, curl, buf, &private_data->data_elf);

    /* Cleanup to gain back memory */
    private_data->curl_easy_cleanup(curl);

    /* Release libcurl and exit thread */
    // TODO: get correct address of OSDynLoad_Release and release the handle
    // finding export is not working
    //void (*OSDynLoad_Release)(unsigned int handle);
    //OSDynLoad_FindExport(private_data->coreinit_handle, 0, "OSDynLoad_Release", &OSDynLoad_Release);
    //OSDynLoad_Release(libcurl_handle);

    /* Pre-load the Mii Studio to be executed on _Exit - thanks to wj444 for sharing it */
    SYSLaunchMiiStudio();
}

static int strcmp(const char *s1, const char *s2)
{
    while(*s1 && *s2)
    {
        if(*s1 != *s2) {
            return -1;
        }
        s1++;
        s2++;
    }

    if(*s1 != *s2) {
        return -1;
    }
    return 0;
}

static unsigned int get_section(private_data_t *private_data, unsigned char *data, const char *name, unsigned int * size, unsigned int * addr)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) data;

    if (   !data
        || !IS_ELF (*ehdr)
        || (ehdr->e_type != ET_EXEC)
        || (ehdr->e_machine != EM_PPC))
    {
        OSFatal("Invalid elf");
    }

    Elf32_Shdr *shdr = (Elf32_Shdr *) (data + ehdr->e_shoff);
    int i;
    for(i = 0; i < ehdr->e_shnum; i++)
    {
        const char *section_name = ((const char*)data) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
        if(strcmp(section_name, name) == 0)
        {
            if(addr)
                *addr = shdr[i].sh_addr;
            if(size)
                *size = shdr[i].sh_size;
            return shdr[i].sh_offset;
        }
    }

    OSFatal((char*)name);
}

/* ****************************************************************** */
/*                         INSTALL MAIN CODE                          */
/* ****************************************************************** */
static void InstallMain(private_data_t *private_data)
{
    // get .kernel_code section
    unsigned int kernel_code_addr = 0;
    unsigned int kernel_code_len = 0;
    unsigned int section_offset = get_section(private_data, private_data->data_elf, ".kernel_code", &kernel_code_len, &kernel_code_addr);
    unsigned char *kernel_code = private_data->data_elf + section_offset;

    /* Copy kernel code section to memory */
    unsigned int cpy_addr = (CODE_RW_BASE_OFFSET + kernel_code_addr);
    memcpy((void*)cpy_addr, kernel_code, kernel_code_len);
    DCFlushRange((void*)cpy_addr, kernel_code_len);
    ICInvalidateRange((void*)cpy_addr, kernel_code_len);

    // get .text section
    unsigned int main_text_addr = 0;
    unsigned int main_text_len = 0;
    section_offset = get_section(private_data, private_data->data_elf, ELF_TEXT, &main_text_len, &main_text_addr);
    unsigned char *main_text = private_data->data_elf + section_offset;
    /* Copy main .text to memory */
    memcpy((void*)(CODE_RW_BASE_OFFSET + main_text_addr), main_text, main_text_len);
    DCFlushRange((void*)(CODE_RW_BASE_OFFSET + main_text_addr), main_text_len);
    ICInvalidateRange((void*)(CODE_RW_BASE_OFFSET + main_text_addr), main_text_len);

    // get the .rodata section
    unsigned int main_rodata_addr = 0;
    unsigned int main_rodata_len = 0;
    section_offset = get_section(private_data, private_data->data_elf, ELF_RODATA, &main_rodata_len, &main_rodata_addr);
    unsigned char *main_rodata = private_data->data_elf + section_offset;
    /* Copy main rodata to memory */
    memcpy((void*)(DATA_RW_BASE_OFFSET + main_rodata_addr), main_rodata, main_rodata_len);
    DCFlushRange((void*)(DATA_RW_BASE_OFFSET + main_rodata_addr), main_rodata_len);

    // get the .bss section
    unsigned int main_bss_addr = 0;
    unsigned int main_bss_len = 0;
    section_offset = get_section(private_data, private_data->data_elf, ELF_BSS, &main_bss_len, &main_bss_addr);
    // Copy main bss to memory
    private_data->memset((void*)(DATA_RW_BASE_OFFSET + main_bss_addr), 0, main_bss_len);
    DCFlushRange((void*)(DATA_RW_BASE_OFFSET + main_bss_addr), main_bss_len);
}

/* ****************************************************************** */
/*                         INSTALL MENU CODE                          */
/* ****************************************************************** */
static void InstallMenu(private_data_t *private_data)
{
    // get .menu_magic section
    unsigned int menu_magic_addr = 0;
    unsigned int menu_magic_len = 0;
    unsigned int section_offset = get_section(private_data, private_data->data_elf, ".menu_magic", &menu_magic_len, &menu_magic_addr);
    unsigned char *menu_magic = private_data->data_elf + section_offset;

    /* Get our functions */
    struct magic_t
    {
        const unsigned int repl_func;     // our replacement function which is called
        const unsigned int repl_addr;     // address where to place the jump to the our function
        const unsigned int call_type;     // call type, e.g. 0x48000000 for branch and 0x48000001 for bl
    } *magic = (struct magic_t *)menu_magic;
    int magic_len = menu_magic_len / sizeof(struct magic_t);

    /* Replace loader instructions */
    /* Loop to replace instructions in loader code by a "bl"(jump) instruction to our replacement function */
    int i;
    for (i = 0; i < magic_len; i ++)
    {
        unsigned int repl_func  = magic[i].repl_func;
        unsigned int repl_addr  = magic[i].repl_addr;
        unsigned int call_type  = magic[i].call_type;

        // Install function hook only if needed
        unsigned int jump_addr = repl_func & 0x03fffffc;                                    // Compute jump length to jump from current instruction address to our function address
        *((volatile uint32_t *)(0xC1000000 + repl_addr)) = call_type | jump_addr;    // Replace the instruction in the loader by the jump to our function
        // flush caches and invalidate instruction cache
        DCFlushRange((void*)(0xC1000000 + repl_addr), 4);
        ICInvalidateRange((void*)(0xC1000000 + repl_addr), 4);
    }
}

/* ****************************************************************** */
/*                              INSTALL LOADER                        */
/* Copy code and replace loader instruction by call to our functions  */
/* ****************************************************************** */
static void InstallLoader(private_data_t *private_data)
{
    // get .magic section
    unsigned int loader_magic_addr = 0;
    unsigned int loader_magic_len = 0;
    unsigned int section_offset = get_section(private_data, private_data->data_elf, ".loader_magic", &loader_magic_len, &loader_magic_addr);
    unsigned char *loader_magic = private_data->data_elf + section_offset;

    /* Get our functions */
    struct magic_t
    {
        const unsigned int repl_func;     // our replacement function which is called
        const unsigned int repl_addr;     // address where to place the jump to the our function
        const unsigned int call_type;     // call type, e.g. 0x48000000 for branch and 0x48000001 for bl
    } *magic = (struct magic_t *)loader_magic;
    int magic_len = loader_magic_len / sizeof(struct magic_t);

    /* Replace loader instructions */
    /* Loop to replace instructions in loader code by a "bl"(jump) instruction to our replacement function */
    int i;
    for (i = 0; i < magic_len; i ++)
    {
        unsigned int repl_func  = magic[i].repl_func;
        unsigned int repl_addr  = magic[i].repl_addr;
        unsigned int call_type  = magic[i].call_type;

        // Install function hook only if needed
        unsigned int jump_addr = repl_func & 0x03fffffc;                                    // Compute jump length to jump from current instruction address to our function address
        *((volatile uint32_t *)(0xC1000000 + repl_addr)) = call_type | jump_addr;    // Replace the instruction in the loader by the jump to our function
        // flush caches and invalidate instruction cache
        DCFlushRange((void*)(0xC1000000 + repl_addr), 4);
        ICInvalidateRange((void*)(0xC1000000 + repl_addr), 4);
    }

    /* Patch to bypass SDK version tests */
    *((volatile uint32_t *)(0xC1000000 + 0x010095b4)) = 0x480000a0; // ble loc_1009654    (0x408100a0) => b loc_1009654      (0x480000a0)
    *((volatile uint32_t *)(0xC1000000 + 0x01009658)) = 0x480000e8; // bge loc_1009740    (0x408100a0) => b loc_1009740      (0x480000e8)
    DCFlushRange((void*)(0xC1000000 + 0x010095b4), 4);
    ICInvalidateRange((void*)(0xC1000000 + 0x010095b4), 4);
    DCFlushRange((void*)(0xC1000000 + 0x01009658), 4);
    ICInvalidateRange((void*)(0xC1000000 + 0x01009658), 4);
}

/* ****************************************************************** */
/*                               INSTALL FS                           */
/* Install filesystem functions                                       */
/* ****************************************************************** */
static void InstallFS(private_data_t *private_data)
{
    // get .magic section
    unsigned int fs_magic_addr = 0;
    unsigned int fs_magic_len = 0;
    unsigned int section_offset = get_section(private_data, private_data->data_elf, ".fs_magic", &fs_magic_len, &fs_magic_addr);
    unsigned char *fs_magic = private_data->data_elf + section_offset;
    // copy the data of the magic for later patching / unpatching
    memcpy((void*)(DATA_RW_BASE_OFFSET + fs_magic_addr), fs_magic, fs_magic_len);
    DCFlushRange((void*)(DATA_RW_BASE_OFFSET + fs_magic_addr), fs_magic_len);
    // get .magic section
    unsigned int magicptr_addr = 0;
    unsigned int magicptr_len = 0;
    section_offset = get_section(private_data, private_data->data_elf, ".magicptr", &magicptr_len, &magicptr_addr);
    unsigned char *magicptr = private_data->data_elf + section_offset;
    // copy the data of the magic for later patching / unpatching
    memcpy((void*)(0xC1000000 + magicptr_addr), magicptr, magicptr_len);
    DCFlushRange((void*)(0xC1000000 + magicptr_addr), magicptr_len);

    // get .magic section
    unsigned int fs_method_calls_addr = 0;
    unsigned int fs_method_calls_len = 0;
    section_offset = get_section(private_data, private_data->data_elf, ".fs_method_calls", &fs_method_calls_len, &fs_method_calls_addr);

    /* ------------------------------------------------------------------------------------------------------------------------*/
    /* patch the FS functions to branch to our functions                                                                       */
    /* ------------------------------------------------------------------------------------------------------------------------*/
    struct magic_t {
        void *real;
        void *replacement;
        void *call;
        unsigned int orig_instr;
    } *magic = (struct magic_t *)fs_magic;
    unsigned int len = fs_magic_len / sizeof(struct magic_t);

    /* Patch branches to it. */
    volatile unsigned int *space = (volatile unsigned int *)(CODE_RW_BASE_OFFSET + fs_method_calls_addr);
    while (len--) {
        unsigned int real_addr = (unsigned int)magic[len].real;
        unsigned int repl_addr = (unsigned int)magic[len].replacement;
        unsigned int call_addr = (unsigned int)magic[len].call;
        unsigned int orig_instr =(unsigned int)magic[len].orig_instr;

        // set pointer to the real function
        *(volatile unsigned int *)(0xC1000000 + call_addr) = (unsigned int)space - CODE_RW_BASE_OFFSET;

        // fill the pointer of the real function
        *space = orig_instr;
        space++;

        // jump to real function skipping the "mflr r0" instruction
        *space = 0x48000002 | ((real_addr + 4) & 0x03fffffc);
        space++;
        DCFlushRange((void*)(space - 2), 8);
        ICInvalidateRange((void*)(space - 2), 8);

        // the installation is done later on during Mii Maker start, only restore here original instruction to "fix" it all up until new patch
        // in the real function, replace the "mflr r0" instruction by a jump to the replacement function
        *(volatile unsigned int *)(0xC1000000 + real_addr) = orig_instr;
        DCFlushRange((unsigned int *)(0xC1000000 + real_addr), 4);
        ICInvalidateRange((unsigned int *)(0xC1000000 + real_addr), 4);
    }
}
